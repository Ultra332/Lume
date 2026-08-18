#include "interpreter.h"
/* Cada chamada de funcao Lume consome varios KB de pilha C, porque o
   interpretador e recursivo. Sem um teto, uma recursao sem caso base derruba o
   processo com SIGSEGV e nenhuma mensagem — justamente o erro que um iniciante
   mais comete ao estudar recursao. O limite transforma isso em diagnostico.
   200 e folgado para exercicios (o fatorial de 20 usa 20 niveis) e fica bem
   abaixo do que a pilha suporta. */
#define LUME_MAX_CALL_DEPTH 200U

#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "callable.h"
#include "runtime_io.h"
#include "list.h"
#include "module.h"

static bool report(ErrorList *errors, LumeErrorKind kind, SourceSpan span,
                   const char *message, const char *suggestion) {
    LumeError error;
    error.kind = kind; error.span = span;
    error.message = message; error.suggestion = suggestion;
    error.subject = NULL; error.subject_length = 0U;
    return error_list_add(errors, error);
}
static bool fail(ErrorList *errors, SourceSpan span, const char *message, const char *suggestion) {
    (void)report(errors, LUME_ERROR_RUNTIME, span, message, suggestion);
    return false;
}
static bool fail_type(ErrorList *errors, SourceSpan span, const char *message, const char *suggestion) {
    (void)report(errors, LUME_ERROR_TYPE, span, message, suggestion);
    return false;
}
static bool fail_numeric(ErrorList *errors, SourceSpan span, const char *message, const char *suggestion) {
    (void)report(errors, LUME_ERROR_NUMERIC, span, message, suggestion);
    return false;
}
static bool fail_kind(ErrorList *errors, LumeErrorKind kind, SourceSpan span, const char *message, const char *suggestion) {
    (void)report(errors, kind, span, message, suggestion); return false;
}
static bool is_number(const Value *value) {
    return value->type == VALUE_INTEGER || value->type == VALUE_DECIMAL;
}
static double as_decimal(const Value *value) {
    return value->type == VALUE_INTEGER ? (double)value->as.integer : value->as.decimal;
}
static bool add_overflows(int64_t left, int64_t right) {
    return (right > 0 && left > INT64_MAX - right) || (right < 0 && left < INT64_MIN - right);
}
static bool subtract_overflows(int64_t left, int64_t right) {
    return (right > 0 && left < INT64_MIN + right) || (right < 0 && left > INT64_MAX + right);
}
static bool multiply_overflows(int64_t left, int64_t right) {
    if (left == 0 || right == 0) return false;
    if (left > 0) {
        if (right > 0) return left > INT64_MAX / right;
        return right < INT64_MIN / left;
    }
    if (right > 0) return left < INT64_MIN / right;
    return right < INT64_MAX / left;
}
static bool numeric_result(double result, Value *out, ErrorList *errors, SourceSpan span) {
    if (!isfinite(result))
        return fail_numeric(errors, span, "O resultado decimal ultrapassa o limite suportado.",
            "Use valores menores nesta operacao.");
    *out = value_decimal(result);
    return true;
}
static bool concatenate(const Value *left, const Value *right, Value *out,
                        ErrorList *errors, SourceSpan span) {
    size_t length;
    char *bytes;
    if (left->as.string.length > SIZE_MAX - right->as.string.length ||
        left->as.string.length + right->as.string.length == SIZE_MAX)
        return fail(errors, span, "O texto resultante e grande demais.", "Use textos menores.");
    length = left->as.string.length + right->as.string.length;
    bytes = memory_allocate(length + 1U);
    if (bytes == NULL)
        return fail(errors, span, "Nao foi possivel reservar memoria para o texto.", "Tente uma expressao menor.");
    if (left->as.string.length > 0U)
        memcpy(bytes, left->as.string.bytes, left->as.string.length);
    if (right->as.string.length > 0U)
        memcpy(bytes + left->as.string.length, right->as.string.bytes, right->as.string.length);
    bytes[length] = '\0';
    out->type = VALUE_STRING; out->as.string.bytes = bytes; out->as.string.length = length;
    return true;
}
static bool values_equal(const Value *left, const Value *right) {
    if (left->type == VALUE_INTEGER && right->type == VALUE_DECIMAL) {
        double decimal = right->as.decimal;
        if (decimal < -9223372036854775808.0 || decimal >= 9223372036854775808.0) return false;
        return (double)(int64_t)decimal == decimal && left->as.integer == (int64_t)decimal;
    }
    if (left->type == VALUE_DECIMAL && right->type == VALUE_INTEGER)
        return values_equal(right, left);
    if (left->type != right->type) return false;
    switch (left->type) {
        case VALUE_NULL: return true;
        case VALUE_BOOLEAN: return left->as.boolean == right->as.boolean;
        case VALUE_INTEGER: return left->as.integer == right->as.integer;
        case VALUE_DECIMAL: return left->as.decimal == right->as.decimal;
        case VALUE_STRING:
            return left->as.string.length == right->as.string.length &&
                memcmp(left->as.string.bytes, right->as.string.bytes, left->as.string.length) == 0;
        case VALUE_CALLABLE: return left->as.callable == right->as.callable;
        case VALUE_LIST: return left->as.list == right->as.list;
        case VALUE_MODULE: return left->as.module == right->as.module;
    }
    return false;
}

typedef struct { RuntimeIO *io; RuntimeTrace *trace; size_t call_depth; ModuleRegistry *registry; LumeModule *module; } Runtime;
typedef enum { EXEC_OK, EXEC_ERROR, EXEC_RETURN } ExecStatus;
typedef struct { ExecStatus status; Value value; } ExecutionResult;
static ExecutionResult execute_statements(const StmtArray *, Environment *, Runtime *, ErrorList *);
static void emit(Runtime *runtime, TraceEvent event) {
    if (runtime->trace != NULL && !runtime->trace->stop_requested &&
        runtime->trace->callback != NULL) {
        event.call_depth = runtime->call_depth;
        runtime->trace->callback(runtime->trace->context, &event);
    }
}
static TraceEvent trace_event(TraceEventType type, SourceSpan span,
                              const Environment *environment) {
    TraceEvent event;
    memset(&event, 0, sizeof(event)); event.type = type; event.span = span;
    event.environment = environment; return event;
}
static bool evaluate(const Expr *expression, Environment *environment, Runtime *runtime, Value *out, ErrorList *errors);
static bool evaluate_unary(const Expr *expression, Environment *environment, Runtime *runtime, Value *out, ErrorList *errors) {
    Value operand = value_null();
    UnaryOperator operator_type = expression->as.unary.operator_type;
    if (!evaluate(expression->as.unary.operand, environment, runtime, &operand, errors)) return false;
    if (operator_type == UNARY_NOT) {
        if (operand.type != VALUE_BOOLEAN) {
            value_free(&operand);
            return fail_type(errors, expression->as.unary.operator_span,
                "O operador 'nao' exige um valor booleano.", "Use verdadeiro ou falso.");
        }
        *out = value_boolean(!operand.as.boolean); value_free(&operand); return true;
    }
    if (!is_number(&operand)) {
        value_free(&operand);
        return fail_type(errors, expression->as.unary.operator_span,
            "Os operadores unarios '+' e '-' exigem um numero.", "Remova o operador ou use um valor numerico.");
    }
    if (operator_type == UNARY_POSITIVE) { *out = operand; return true; }
    if (operand.type == VALUE_INTEGER) {
        if (operand.as.integer == INT64_MIN) {
            value_free(&operand);
            return fail_numeric(errors, expression->span, "O resultado ultrapassa o limite de inteiro da Lume.",
                "Use um valor menor.");
        }
        *out = value_integer(-operand.as.integer);
    } else {
        if (!numeric_result(-operand.as.decimal, out, errors, expression->span)) {
            value_free(&operand); return false;
        }
    }
    value_free(&operand); return true;
}
static bool evaluate_numeric_binary(BinaryOperator operator_type, const Value *left, const Value *right,
                                    Value *out, ErrorList *errors, SourceSpan span) {
    if (operator_type == BINARY_DIVIDE) {
        double divisor = as_decimal(right);
        if (divisor == 0.0) return fail_numeric(errors, span, "Nao e possivel dividir por zero.", "Use um divisor diferente de zero.");
        return numeric_result(as_decimal(left) / divisor, out, errors, span);
    }
    if (operator_type == BINARY_REMAINDER) {
        if (left->type != VALUE_INTEGER || right->type != VALUE_INTEGER)
            return fail_type(errors, span, "O operador '%' aceita apenas inteiros.", "Use dois valores inteiros.");
        if (right->as.integer == 0)
            return fail_numeric(errors, span, "Nao e possivel calcular resto por zero.", "Use um divisor diferente de zero.");
        if (left->as.integer == INT64_MIN && right->as.integer == -1) {
            *out = value_integer(0); return true;
        }
        *out = value_integer(left->as.integer % right->as.integer); return true;
    }
    if (left->type == VALUE_DECIMAL || right->type == VALUE_DECIMAL) {
        double left_number = as_decimal(left), right_number = as_decimal(right), result;
        if (operator_type == BINARY_ADD) result = left_number + right_number;
        else if (operator_type == BINARY_SUBTRACT) result = left_number - right_number;
        else result = left_number * right_number;
        return numeric_result(result, out, errors, span);
    }
    if (operator_type == BINARY_ADD) {
        if (add_overflows(left->as.integer, right->as.integer))
            return fail_numeric(errors, span, "A soma ultrapassa o limite de inteiro da Lume.", "Use valores menores.");
        *out = value_integer(left->as.integer + right->as.integer); return true;
    }
    if (operator_type == BINARY_SUBTRACT) {
        if (subtract_overflows(left->as.integer, right->as.integer))
            return fail_numeric(errors, span, "A subtracao ultrapassa o limite de inteiro da Lume.", "Use valores menores.");
        *out = value_integer(left->as.integer - right->as.integer); return true;
    }
    if (multiply_overflows(left->as.integer, right->as.integer))
        return fail_numeric(errors, span, "A multiplicacao ultrapassa o limite de inteiro da Lume.", "Use valores menores.");
    *out = value_integer(left->as.integer * right->as.integer); return true;
}
static bool evaluate_binary(const Expr *expression, Environment *environment, Runtime *runtime, Value *out, ErrorList *errors) {
    BinaryOperator operator_type = expression->as.binary.operator_type;
    Value left = value_null(), right = value_null();
    bool ok;
    if (!evaluate(expression->as.binary.left, environment, runtime, &left, errors)) return false;
    if (operator_type == BINARY_LOGICAL_AND || operator_type == BINARY_LOGICAL_OR) {
        if (left.type != VALUE_BOOLEAN) {
            value_free(&left);
            return fail_type(errors, expression->as.binary.operator_span,
                "Os operadores 'e' e 'ou' exigem booleanos.", "Use verdadeiro ou falso nos dois lados.");
        }
        if ((operator_type == BINARY_LOGICAL_AND && !left.as.boolean) ||
            (operator_type == BINARY_LOGICAL_OR && left.as.boolean)) {
            *out = value_boolean(left.as.boolean); value_free(&left); return true;
        }
    }
    if (!evaluate(expression->as.binary.right, environment, runtime, &right, errors)) { value_free(&left); return false; }
    if (operator_type == BINARY_LOGICAL_AND || operator_type == BINARY_LOGICAL_OR) {
        if (right.type != VALUE_BOOLEAN) {
            value_free(&left); value_free(&right);
            return fail_type(errors, expression->as.binary.operator_span,
                "Os operadores 'e' e 'ou' exigem booleanos.", "Use verdadeiro ou falso nos dois lados.");
        }
        *out = value_boolean(operator_type == BINARY_LOGICAL_AND ?
            left.as.boolean && right.as.boolean : left.as.boolean || right.as.boolean);
        value_free(&left); value_free(&right); return true;
    }
    if (operator_type == BINARY_EQUAL || operator_type == BINARY_NOT_EQUAL) {
        bool equal = values_equal(&left, &right);
        *out = value_boolean(operator_type == BINARY_EQUAL ? equal : !equal);
        value_free(&left); value_free(&right); return true;
    }
    if (operator_type == BINARY_LESS || operator_type == BINARY_LESS_EQUAL ||
        operator_type == BINARY_GREATER || operator_type == BINARY_GREATER_EQUAL) {
        double left_number, right_number;
        if (!is_number(&left) || !is_number(&right)) {
            value_free(&left); value_free(&right);
            return fail_type(errors, expression->as.binary.operator_span,
                "Comparacoes de ordem exigem dois numeros.", "Use inteiros ou decimais nos dois lados.");
        }
        left_number = as_decimal(&left); right_number = as_decimal(&right);
        if (operator_type == BINARY_LESS) ok = left_number < right_number;
        else if (operator_type == BINARY_LESS_EQUAL) ok = left_number <= right_number;
        else if (operator_type == BINARY_GREATER) ok = left_number > right_number;
        else ok = left_number >= right_number;
        *out = value_boolean(ok); value_free(&left); value_free(&right); return true;
    }
    if (operator_type == BINARY_ADD && left.type == VALUE_STRING && right.type == VALUE_STRING) {
        ok = concatenate(&left, &right, out, errors, expression->span);
        value_free(&left); value_free(&right); return ok;
    }
    if (!is_number(&left) || !is_number(&right)) {
        value_free(&left); value_free(&right);
        return fail_type(errors, expression->as.binary.operator_span,
            "Este operador aritmetico exige dois numeros do mesmo dominio compativel.",
            "Use dois numeros; apenas '+' tambem aceita dois textos.");
    }
    ok = evaluate_numeric_binary(operator_type, &left, &right, out, errors, expression->span);
    value_free(&left); value_free(&right); return ok;
}
static bool call_callable(Callable *callable, Value *arguments, size_t count,
                          Environment *environment, Runtime *runtime,
                          Value *out, SourceSpan span, ErrorList *errors) {
    size_t index;
    if (count != callable->arity)
        return fail_kind(errors, LUME_ERROR_CALL, span, "Quantidade incorreta de argumentos na chamada.",
            "Confira a quantidade de parametros da funcao.");
    if (callable->type != CALLABLE_USER) {
        TraceEvent event = trace_event(TRACE_NATIVE_CALL, span, environment);
        event.name = callable->name; event.name_length = strlen(callable->name);
        event.arguments = arguments; event.argument_count = count; emit(runtime, event);
    }
    if(callable->type==CALLABLE_NATIVE_CUSTOM)return callable->native_function(arguments,count,runtime->io,callable->native_context,out,span,errors);
    if (callable->type == CALLABLE_NATIVE_WRITE) {
        TraceEvent event = trace_event(TRACE_OUTPUT, span, environment);
        event.name = "escreva"; event.name_length = 7U; event.after = &arguments[0]; emit(runtime, event);
        value_print(runtime->io->output, &arguments[0]);
        fputc('\n', runtime->io->output); *out = value_null(); return true;
    }
    if (callable->type == CALLABLE_NATIVE_READ) {
        char *bytes = NULL; size_t length = 0U, capacity = 0U; int character;
        while ((character = fgetc(runtime->io->input)) != EOF && character != '\n') {
            char *grown; size_t new_capacity;
            if (length == capacity) {
                if (!memory_grow_capacity(capacity, length + 1U, &new_capacity)) goto read_memory;
                grown = memory_reallocate_array(bytes, new_capacity, sizeof(*grown));
                if (grown == NULL) goto read_memory;
                bytes = grown; capacity = new_capacity;
            }
            bytes[length++] = (char)character;
        }
        if (character == EOF && length == 0U) { memory_free(bytes); *out = value_null(); return true; }
        if (length > 0U && bytes[length - 1U] == '\r') length--;
        if (!value_string_copy(bytes, length, out)) goto read_memory;
        memory_free(bytes); return true;
read_memory:
        memory_free(bytes); return fail(errors, span, "Nao foi possivel ler a entrada.", "Tente uma linha menor.");
    }
    if (callable->type == CALLABLE_NATIVE_TEXT) {
        char *bytes; size_t length;
        if (!value_format(&arguments[0], &bytes, &length))
            return fail(errors, span, "Nao foi possivel converter o valor para texto.", "Tente um valor menor.");
        out->type = VALUE_STRING; out->as.string.bytes = bytes; out->as.string.length = length; return true;
    }
    if (callable->type == CALLABLE_NATIVE_TYPE)
        return value_string_copy(value_type_name(arguments[0].type), strlen(value_type_name(arguments[0].type)), out);
    if (callable->type == CALLABLE_NATIVE_LENGTH) {
        if(arguments[0].type==VALUE_LIST){if(arguments[0].as.list->count>(size_t)INT64_MAX)return fail_numeric(errors,span,"A lista e grande demais para representar seu tamanho.","Use uma lista menor.");*out=value_integer((int64_t)arguments[0].as.list->count);return true;}
        if(arguments[0].type==VALUE_STRING){size_t at=0U,points=0U;const unsigned char *bytes=(const unsigned char *)arguments[0].as.string.bytes;size_t length=arguments[0].as.string.length;
            while(at<length){size_t needed;unsigned char c=bytes[at];if(c<0x80U)needed=1U;else if(c>=0xC2U&&c<=0xDFU)needed=2U;else if(c>=0xE0U&&c<=0xEFU)needed=3U;else if(c>=0xF0U&&c<=0xF4U)needed=4U;else return fail_type(errors,span,"O texto contem UTF-8 invalido.","Use texto UTF-8 valido.");if(needed>length-at)return fail_type(errors,span,"O texto termina no meio de um caractere UTF-8.","Use texto UTF-8 valido.");{size_t j;for(j=1U;j<needed;j++)if((bytes[at+j]&0xC0U)!=0x80U)return fail_type(errors,span,"O texto contem UTF-8 invalido.","Use texto UTF-8 valido.");}if((c==0xE0U&&bytes[at+1U]<0xA0U)||(c==0xEDU&&bytes[at+1U]>0x9FU)||(c==0xF0U&&bytes[at+1U]<0x90U)||(c==0xF4U&&bytes[at+1U]>0x8FU))return fail_type(errors,span,"O texto contem um ponto de codigo UTF-8 invalido.","Use texto UTF-8 valido.");at+=needed;points++;}
            if(points>(size_t)INT64_MAX)return fail_numeric(errors,span,"O texto e grande demais.","Use um texto menor.");
            *out=value_integer((int64_t)points);return true;}
        return fail_type(errors,span,"'tamanho' aceita somente lista ou texto.","Use tamanho(lista) ou tamanho(texto).");
    }
    if(callable->type==CALLABLE_NATIVE_APPEND){TraceEvent event;if(arguments[0].type!=VALUE_LIST)return fail_type(errors,span,"'adicione' exige uma lista no primeiro argumento.","Use adicione(lista, valor).");if(list_would_create_cycle(arguments[0].as.list,&arguments[1]))return fail(errors,span,"Esta operacao criaria uma lista ciclica.","Listas ciclicas nao sao permitidas nesta versao.");if(!list_append(arguments[0].as.list,&arguments[1]))return fail(errors,span,"Nao foi possivel ampliar a lista.","Tente uma lista menor.");event=trace_event(TRACE_LIST_APPEND,span,environment);event.after=&arguments[0];emit(runtime,event);*out=value_null();return true;}
    if(callable->type==CALLABLE_NATIVE_REMOVE){TraceEvent event;if(arguments[0].type!=VALUE_LIST)return fail_type(errors,span,"'remova' exige uma lista no primeiro argumento.","Use remova(lista, indice).");if(arguments[1].type!=VALUE_INTEGER)return fail_kind(errors,LUME_ERROR_INDEX,span,"O indice de 'remova' precisa ser inteiro.","Use um indice a partir de zero.");if(arguments[1].as.integer<0||(uint64_t)arguments[1].as.integer>=(uint64_t)arguments[0].as.list->count)return fail_kind(errors,LUME_ERROR_INDEX,span,"O indice de remocao nao existe nesta lista.","Use um indice entre zero e tamanho(lista) - 1.");if(!list_remove(arguments[0].as.list,(size_t)arguments[1].as.integer,out))return false;event=trace_event(TRACE_LIST_REMOVE,span,environment);event.index=arguments[1].as.integer;event.after=out;emit(runtime,event);return true;}
    if (callable->type == CALLABLE_NATIVE_INTEGER) {
        const Value *argument = &arguments[0];
        if (argument->type == VALUE_INTEGER) { *out = value_integer(argument->as.integer); return true; }
        if (argument->type == VALUE_DECIMAL) {
            double number = argument->as.decimal;
            if (!isfinite(number) || number < -9223372036854775808.0 || number >= 9223372036854775808.0 || trunc(number) != number)
                return fail_kind(errors, LUME_ERROR_CONVERSION, span, "A conversao para inteiro exige um numero sem parte fracionaria.",
                    "Use um decimal integral, como 10.0.");
            *out = value_integer((int64_t)number); return true;
        }
        if (argument->type == VALUE_STRING) {
            const char *bytes = argument->as.string.bytes; size_t length = argument->as.string.length;
            size_t at = 0U; bool negative = false; uint64_t number = 0U, limit;
            if (length > 0U && (bytes[0] == '-' || bytes[0] == '+')) { negative = bytes[0] == '-'; at++; }
            if (at == length) goto integer_error;
            limit = negative ? UINT64_C(9223372036854775808) : (uint64_t)INT64_MAX;
            for (; at < length; at++) {
                unsigned digit;
                if (bytes[at] < '0' || bytes[at] > '9') goto integer_error;
                digit = (unsigned)((unsigned char)bytes[at] - (unsigned char)'0');
                if (number > (limit - digit) / 10U) goto integer_error;
                number = number * 10U + digit;
            }
            if (negative && number == UINT64_C(9223372036854775808)) *out = value_integer(INT64_MIN);
            else *out = value_integer(negative ? -(int64_t)number : (int64_t)number);
            return true;
        }
integer_error:
        return fail_kind(errors, LUME_ERROR_CONVERSION, span, "Nao foi possivel converter o valor para inteiro.",
            "Use um inteiro, um decimal integral ou texto contendo somente um inteiro valido.");
    }
    if (callable->type == CALLABLE_NATIVE_DECIMAL) {
        const Value *argument = &arguments[0];
        if (argument->type == VALUE_DECIMAL) { *out = value_decimal(argument->as.decimal); return true; }
        if (argument->type == VALUE_INTEGER) { *out = value_decimal((double)argument->as.integer); return true; }
        if (argument->type == VALUE_STRING) {
            char *end; double number; size_t at = 0U; bool dot = false; size_t digits_before = 0U, digits_after = 0U;
            if (argument->as.string.length == 0U) goto decimal_error;
            if (argument->as.string.bytes[at] == '+' || argument->as.string.bytes[at] == '-') at++;
            for (; at < argument->as.string.length; at++) {
                char character = argument->as.string.bytes[at];
                if (character == '.' && !dot) { dot = true; continue; }
                if (character < '0' || character > '9') goto decimal_error;
                if (dot) digits_after++; else digits_before++;
            }
            if (digits_before == 0U || (dot && digits_after == 0U)) goto decimal_error;
            errno = 0; number = strtod(argument->as.string.bytes, &end);
            if (errno == ERANGE || !isfinite(number) || end != argument->as.string.bytes + argument->as.string.length)
                goto decimal_error;
            *out = value_decimal(number); return true;
        }
decimal_error:
        return fail_kind(errors, LUME_ERROR_CONVERSION, span, "Nao foi possivel converter o valor para decimal.",
            "Use um numero ou texto contendo somente um decimal valido.");
    }
    {
        const Stmt *declaration = callable->declaration;
        Environment *call_environment = environment_new_child(callable->closure);
        ExecutionResult result;
        TraceEvent call_event = trace_event(TRACE_FUNCTION_CALL, span, environment);
        call_event.name=callable->name;call_event.name_length=strlen(callable->name);
        call_event.arguments=arguments;call_event.argument_count=count;emit(runtime,call_event);
        if (call_environment == NULL)
            return fail(errors, span, "Nao foi possivel criar o ambiente da chamada.", "Tente um programa menor.");
        for (index = 0U; index < count; index++) {
            if (!environment_define(call_environment, declaration->as.function.parameters[index],
                    declaration->as.function.parameter_lengths[index], &arguments[index], true,
                    declaration->as.function.parameter_spans[index], errors)) return false;
        }
        if (runtime->call_depth >= LUME_MAX_CALL_DEPTH) {
            return fail(errors, span,
                "A funcao chamou a si mesma vezes demais e a execucao foi interrompida.",
                "Toda recursao precisa de um caso base que pare as chamadas. Confira se a condicao de parada e alcancada.");
        }
        runtime->call_depth++;
        { TraceEvent enter_event=trace_event(TRACE_FUNCTION_ENTER,span,call_environment);enter_event.name=callable->name;enter_event.name_length=strlen(callable->name);emit(runtime,enter_event); }
        result = execute_statements(&declaration->as.function.body->as.block.statements,
            call_environment, runtime, errors);
        if (result.status == EXEC_ERROR) { runtime->call_depth--; return false; }
        if (result.status == EXEC_RETURN) { TraceEvent return_event=trace_event(TRACE_FUNCTION_RETURN,span,call_environment);return_event.name=callable->name;return_event.name_length=strlen(callable->name);return_event.after=&result.value;emit(runtime,return_event);runtime->call_depth--;*out=result.value;return true; }
        *out=value_null();{TraceEvent return_event=trace_event(TRACE_FUNCTION_RETURN,span,call_environment);return_event.name=callable->name;return_event.name_length=strlen(callable->name);return_event.after=out;emit(runtime,return_event);}runtime->call_depth--;return true;
    }
}
static bool evaluate_call(const Expr *expression, Environment *environment, Runtime *runtime,
                          Value *out, ErrorList *errors) {
    Value callee = value_null(); Value *arguments = NULL; size_t index;
    bool ok;
    if (!evaluate(expression->as.call.callee, environment, runtime, &callee, errors)) return false;
    if (callee.type != VALUE_CALLABLE) {
        value_free(&callee);
        return fail_type(errors, expression->as.call.callee->span, "Este valor nao pode ser chamado.",
            "Chame uma funcao ou uma funcao nativa.");
    }
    if (expression->as.call.argument_count > 0U) {
        arguments = memory_reallocate_array(NULL, expression->as.call.argument_count, sizeof(*arguments));
        if (arguments == NULL) { value_free(&callee); return fail(errors, expression->span, "Nao foi possivel reservar os argumentos.", "Tente menos argumentos."); }
        for (index = 0U; index < expression->as.call.argument_count; index++) arguments[index] = value_null();
    }
    for (index = 0U; index < expression->as.call.argument_count; index++) {
        if (!evaluate(expression->as.call.arguments[index], environment, runtime, &arguments[index], errors)) {
            while (index > 0U) value_free(&arguments[--index]);
            memory_free(arguments); value_free(&callee); return false;
        }
    }
    ok = call_callable(callee.as.callable, arguments, expression->as.call.argument_count,
        environment, runtime, out, expression->span, errors);
    for (index = 0U; index < expression->as.call.argument_count; index++) value_free(&arguments[index]);
    memory_free(arguments); value_free(&callee); return ok;
}
static bool evaluate(const Expr *expression, Environment *environment, Runtime *runtime, Value *out, ErrorList *errors) {
    if (expression == NULL) return false;
    switch (expression->type) {
        case EXPR_LITERAL:
            if (!value_copy(&expression->as.literal, out))
                return fail(errors, expression->span, "Nao foi possivel copiar o valor.", "Tente uma expressao menor.");
            return true;
        case EXPR_IDENTIFIER:
            return environment_get(environment, expression->as.identifier.name,
                expression->as.identifier.length, out, expression->span, errors);
        case EXPR_GROUPING: return evaluate(expression->as.grouping.expression, environment, runtime, out, errors);
        case EXPR_UNARY: return evaluate_unary(expression, environment, runtime, out, errors);
        case EXPR_BINARY: return evaluate_binary(expression, environment, runtime, out, errors);
        case EXPR_CALL: return evaluate_call(expression, environment, runtime, out, errors);
        case EXPR_LIST: {
            LumeList *list=list_new(); size_t index;
            if(list==NULL)return fail(errors,expression->span,"Nao foi possivel criar a lista.","Tente uma lista menor.");
            for(index=0U;index<expression->as.list.count;index++){Value item=value_null();if(!evaluate(expression->as.list.elements[index],environment,runtime,&item,errors)){list_release(list);return false;}if(!list_append(list,&item)){value_free(&item);list_release(list);return fail(errors,expression->span,"Nao foi possivel adicionar um elemento a lista.","A lista nao pode conter um ciclo consigo mesma.");}value_free(&item);}
            *out=value_list(list);{TraceEvent event=trace_event(TRACE_LIST_CREATE,expression->span,environment);event.after=out;emit(runtime,event);}return true;
        }
        case EXPR_INDEX: {
            Value target=value_null(),index=value_null();bool ok;
            if(!evaluate(expression->as.index.target,environment,runtime,&target,errors))return false;
            if(!evaluate(expression->as.index.index,environment,runtime,&index,errors)){value_free(&target);return false;}
            if(target.type!=VALUE_LIST){value_free(&target);value_free(&index);return fail_kind(errors,LUME_ERROR_INDEX,expression->span,"Este valor nao pode ser indexado.","Valores indexaveis atualmente: lista.");}
            if(index.type!=VALUE_INTEGER){value_free(&target);value_free(&index);return fail_kind(errors,LUME_ERROR_INDEX,expression->as.index.index->span,"O indice de uma lista precisa ser inteiro.","Use um inteiro a partir de zero.");}
            if(index.as.integer<0||(uint64_t)index.as.integer>=(uint64_t)target.as.list->count){value_free(&target);value_free(&index);return fail_kind(errors,LUME_ERROR_INDEX,expression->as.index.index->span,"O indice nao existe nesta lista.","Indices comecam em zero e precisam ser menores que tamanho(lista).");}
            ok=list_get(target.as.list,(size_t)index.as.integer,out);if(ok){TraceEvent event=trace_event(TRACE_INDEX_READ,expression->span,environment);event.index=index.as.integer;event.after=out;emit(runtime,event);}value_free(&target);value_free(&index);return ok;
        }
        case EXPR_MEMBER: {
            Value target=value_null();bool ok;
            if(!evaluate(expression->as.member.target,environment,runtime,&target,errors))return false;
            if(target.type!=VALUE_MODULE){value_free(&target);return fail_type(errors,expression->span,"Acesso por membro exige um modulo.","Use modulo.nome depois de importar o modulo.");}
            ok=module_get_export(target.as.module,expression->as.member.name,expression->as.member.name_length,out,expression->as.member.name_span,errors);value_free(&target);return ok;
        }
    }
    return fail(errors, expression->span, "Tipo interno de expressao desconhecido.", "Isto indica um erro interno da Lume.");
}
bool interpreter_evaluate_expression(const Expr *expression, Value *out, ErrorList *errors) {
    RuntimeIO io; Runtime runtime;
    if (expression == NULL || out == NULL || errors == NULL || errors->count != 0U) return false;
    *out = value_null();
    runtime_io_default(&io); runtime.io = &io; runtime.trace = NULL; runtime.call_depth=0U;runtime.registry=NULL;runtime.module=NULL;
    return evaluate(expression, NULL, &runtime, out, errors);
}
bool interpreter_evaluate_expression_in_environment(const Expr *expression,
    Environment *environment, Value *out, ErrorList *errors) {
    RuntimeIO io; Runtime runtime;
    if (expression == NULL || environment == NULL || out == NULL || errors == NULL ||
        errors->count != 0U) return false;
    *out = value_null();
    runtime_io_default(&io); runtime.io = &io; runtime.trace = NULL; runtime.call_depth=0U;runtime.registry=NULL;runtime.module=NULL;
    return evaluate(expression, environment, &runtime, out, errors);
}
bool interpreter_evaluate_expression_with_io(const Expr *expression, Environment *environment,
    RuntimeIO *io, Value *out, ErrorList *errors) {
    Runtime runtime;
    if (expression == NULL || environment == NULL || io == NULL || out == NULL ||
        errors == NULL || errors->count != 0U) return false;
    runtime.io = io; runtime.trace = NULL; runtime.call_depth=0U;runtime.registry=NULL;runtime.module=NULL; *out = value_null();
    return evaluate(expression, environment, &runtime, out, errors);
}
static bool evaluate_condition(const Expr *condition, Environment *environment,
                               Runtime *runtime, Value *result, ErrorList *errors, const char *message) {
    if (!evaluate(condition, environment, runtime, result, errors)) return false;
    if (result->type != VALUE_BOOLEAN) {
        value_free(result);
        return fail_type(errors, condition->span, message,
            "Use uma comparacao ou os valores verdadeiro e falso.");
    }
    return true;
}
static ExecutionResult execution(ExecStatus status) {
    ExecutionResult result; result.status = status; result.value = value_null(); return result;
}
static ExecutionResult execute_statement(const Stmt *statement, Environment *environment,
                                         Runtime *runtime, ErrorList *errors) {
    Value value = value_null();
    bool ok;
    switch (statement->type) {
        case STMT_EXPRESSION:
            ok = evaluate(statement->as.expression.expression, environment, runtime, &value, errors);
            value_free(&value);
            return execution(ok ? EXEC_OK : EXEC_ERROR);
        case STMT_VARIABLE_DECLARATION:
        case STMT_CONSTANT_DECLARATION:
            if (!environment_validate_definition(environment, statement->as.declaration.name,
                    statement->as.declaration.name_length,
                    statement->as.declaration.name_span, errors)) return execution(EXEC_ERROR);
            if (!evaluate(statement->as.declaration.initializer, environment, runtime, &value, errors))
                return execution(EXEC_ERROR);
            ok = environment_define(environment, statement->as.declaration.name,
                statement->as.declaration.name_length, &value,
                statement->type == STMT_VARIABLE_DECLARATION,
                statement->as.declaration.name_span, errors);
            value_free(&value);
            if (ok) { Value current = value_null();
                if (environment_get(environment, statement->as.declaration.name, statement->as.declaration.name_length, &current, statement->span, errors)) {
                    TraceEvent event=trace_event(statement->type==STMT_VARIABLE_DECLARATION?TRACE_DECLARE_VARIABLE:TRACE_DECLARE_CONSTANT,statement->span,environment);
                    event.name=statement->as.declaration.name;event.name_length=statement->as.declaration.name_length;event.after=&current;emit(runtime,event);value_free(&current);
                }
            }
            return execution(ok ? EXEC_OK : EXEC_ERROR);
        case STMT_ASSIGNMENT:
            {
            Value before = value_null();
            (void)environment_get(environment, statement->as.assignment.name, statement->as.assignment.name_length, &before, statement->span, errors);
            if (errors->count > 0U) { value_free(&before); return execution(EXEC_ERROR); }
            if (!evaluate(statement->as.assignment.value, environment, runtime, &value, errors))
                { value_free(&before); return execution(EXEC_ERROR); }
            ok = environment_assign(environment, statement->as.assignment.name,
                statement->as.assignment.name_length, &value,
                statement->as.assignment.name_span, errors);
            value_free(&value);
            if (ok) { Value after = value_null(); (void)environment_get(environment, statement->as.assignment.name, statement->as.assignment.name_length, &after, statement->span, errors);
                TraceEvent event=trace_event(TRACE_ASSIGN,statement->span,environment);event.name=statement->as.assignment.name;event.name_length=statement->as.assignment.name_length;event.before=&before;event.after=&after;emit(runtime,event);value_free(&after); }
            value_free(&before);
            return execution(ok ? EXEC_OK : EXEC_ERROR);
            }
        case STMT_BLOCK: {
            Environment *child = environment_new_child(environment);
            if (child == NULL) {
                (void)fail(errors, statement->span, "Nao foi possivel criar o escopo do bloco.", "Tente um programa menor.");
                return execution(EXEC_ERROR);
            }
            return execute_statements(&statement->as.block.statements, child, runtime, errors);
        }
        case STMT_IF:
            if (!evaluate_condition(statement->as.if_statement.condition, environment,
                    runtime, &value, errors, "A condicao de 'se' precisa ser booleana.")) return execution(EXEC_ERROR);
            ok = value.as.boolean;
            { TraceEvent event=trace_event(TRACE_IF_CONDITION,statement->as.if_statement.condition->span,environment);event.name="se";event.name_length=2U;event.decision=ok;emit(runtime,event); }
            value_free(&value);
            if (ok) return execute_statement(statement->as.if_statement.then_branch, environment, runtime, errors);
            if (statement->as.if_statement.else_branch != NULL)
                return execute_statement(statement->as.if_statement.else_branch, environment, runtime, errors);
            return execution(EXEC_OK);
        case STMT_WHILE: {
            size_t iteration = 0U;
            for (;;) {
                if (!evaluate_condition(statement->as.while_statement.condition, environment,
                        runtime, &value, errors, "A condicao de 'enquanto' precisa ser booleana.")) return execution(EXEC_ERROR);
                ok = value.as.boolean;
                {TraceEvent event=trace_event(TRACE_WHILE_CONDITION,statement->as.while_statement.condition->span,environment);event.name="enquanto";event.name_length=8U;event.decision=ok;event.iteration=iteration;emit(runtime,event);}
                value_free(&value);
                if (!ok) {TraceEvent event=trace_event(TRACE_WHILE_END,statement->span,environment);event.iteration=iteration;emit(runtime,event);return execution(EXEC_OK);}
                iteration++; {TraceEvent event=trace_event(TRACE_WHILE_ITERATION,statement->span,environment);event.name="enquanto";event.name_length=8U;event.iteration=iteration;emit(runtime,event);}
                {
                    ExecutionResult result = execute_statement(statement->as.while_statement.body, environment, runtime, errors);
                    if (result.status != EXEC_OK) return result;
                }
            }
        }
        case STMT_FOR: {
            Value start = value_null();
            Value end = value_null();
            Environment *loop_environment;
            int64_t current;
            int64_t final_value;
            size_t iteration = 0U;
            if (!evaluate(statement->as.for_statement.start, environment, runtime, &start, errors))
                return execution(EXEC_ERROR);
            if (!evaluate(statement->as.for_statement.end, environment, runtime, &end, errors)) {
                value_free(&start); return execution(EXEC_ERROR);
            }
            if (start.type != VALUE_INTEGER || end.type != VALUE_INTEGER) {
                value_free(&start); value_free(&end);
                (void)fail_type(errors, statement->span,
                    "Os limites de 'para' precisam ser inteiros.",
                    "Use valores inteiros depois de 'de' e 'ate'.");
                return execution(EXEC_ERROR);
            }
            current = start.as.integer; final_value = end.as.integer;
            {TraceEvent event=trace_event(TRACE_FOR_START,statement->span,environment);event.name=statement->as.for_statement.iterator_name;event.name_length=statement->as.for_statement.iterator_length;event.before=&start;event.after=&end;emit(runtime,event);}
            value_free(&start); value_free(&end);
            if (current > final_value) {TraceEvent event=trace_event(TRACE_FOR_END,statement->span,environment);emit(runtime,event);return execution(EXEC_OK);}
            loop_environment = environment_new_child(environment);
            if (loop_environment == NULL) return execution(EXEC_ERROR);
            value = value_integer(current);
            if (!environment_define(loop_environment,
                    statement->as.for_statement.iterator_name,
                    statement->as.for_statement.iterator_length, &value, false,
                    statement->as.for_statement.iterator_span, errors)) {
                return execution(EXEC_ERROR);
            }
            for (;;) {
                iteration++; {TraceEvent event=trace_event(TRACE_FOR_ITERATION,statement->span,loop_environment);Value iterator=value_integer(current);event.name=statement->as.for_statement.iterator_name;event.name_length=statement->as.for_statement.iterator_length;event.iteration=iteration;event.after=&iterator;emit(runtime,event);}
                ExecutionResult result = execute_statement(statement->as.for_statement.body, loop_environment, runtime, errors);
                if (result.status != EXEC_OK) {
                    return result;
                }
                if (current == final_value) break;
                current++;
                value = value_integer(current);
                if (!environment_set_local_internal(loop_environment,
                        statement->as.for_statement.iterator_name,
                        statement->as.for_statement.iterator_length, &value)) {
                    (void)fail(errors, statement->as.for_statement.iterator_span,
                        "Nao foi possivel atualizar o iterador interno do laco.",
                        "Isto indica falta de memoria ou erro interno.");
                    return execution(EXEC_ERROR);
                }
            }
            {TraceEvent event=trace_event(TRACE_FOR_END,statement->span,environment);event.iteration=iteration;emit(runtime,event);}return execution(EXEC_OK);
        }
        case STMT_FUNCTION: return execution(EXEC_OK);
        case STMT_RETURN: {
            ExecutionResult result = execution(EXEC_RETURN);
            if (statement->as.return_statement.value != NULL &&
                !evaluate(statement->as.return_statement.value, environment, runtime, &result.value, errors))
                return execution(EXEC_ERROR);
            return result;
        }
        case STMT_INDEX_ASSIGNMENT: {
            Value target=value_null(),index=value_null(),assigned=value_null(),before=value_null();
            if(!evaluate(statement->as.index_assignment.target,environment,runtime,&target,errors))return execution(EXEC_ERROR);
            if(!evaluate(statement->as.index_assignment.index,environment,runtime,&index,errors)){value_free(&target);return execution(EXEC_ERROR);}
            if(target.type!=VALUE_LIST){value_free(&target);value_free(&index);(void)fail_kind(errors,LUME_ERROR_INDEX,statement->span,"Este valor nao permite alteracao por indice.","Somente listas podem ter elementos alterados.");return execution(EXEC_ERROR);}
            if(index.type!=VALUE_INTEGER||index.as.integer<0||(uint64_t)index.as.integer>=(uint64_t)target.as.list->count){value_free(&target);value_free(&index);(void)fail_kind(errors,LUME_ERROR_INDEX,statement->span,"Indice invalido para alteracao da lista.","Use um inteiro entre zero e tamanho(lista) - 1.");return execution(EXEC_ERROR);}
            if(!evaluate(statement->as.index_assignment.value,environment,runtime,&assigned,errors)){value_free(&target);value_free(&index);return execution(EXEC_ERROR);}
            if(list_would_create_cycle(target.as.list,&assigned)){value_free(&target);value_free(&index);value_free(&assigned);(void)fail(errors,statement->span,"Esta alteracao criaria uma lista ciclica.","Listas ciclicas nao sao permitidas nesta versao.");return execution(EXEC_ERROR);}
            if(!list_get(target.as.list,(size_t)index.as.integer,&before)){value_free(&target);value_free(&index);value_free(&assigned);return execution(EXEC_ERROR);}
            if(!list_set(target.as.list,(size_t)index.as.integer,&assigned)){value_free(&before);value_free(&target);value_free(&index);value_free(&assigned);return execution(EXEC_ERROR);}
            {TraceEvent event=trace_event(TRACE_INDEX_WRITE,statement->span,environment);event.index=index.as.integer;event.before=&before;event.after=&assigned;emit(runtime,event);}value_free(&before);value_free(&target);value_free(&index);value_free(&assigned);return execution(EXEC_OK);
        }
        case STMT_IMPORT: {
            LumeModule *imported;Value module_value=value_null();bool replace_native=statement->as.import.path_length==10U&&memcmp(statement->as.import.path,"lume/texto",10U)==0;
            if(runtime->registry==NULL){(void)fail(errors,statement->span,"Imports exigem uma execucao associada a um arquivo.","Execute o programa pela CLI ou pelo REPL.");return execution(EXEC_ERROR);}
            if(environment_has_current(environment,statement->as.import.binding,statement->as.import.binding_length)){if(environment_get(environment,statement->as.import.binding,statement->as.import.binding_length,&module_value,statement->span,errors)&&module_value.type==VALUE_MODULE){value_free(&module_value);return execution(EXEC_OK);}if(!(replace_native&&module_value.type==VALUE_CALLABLE&&module_value.as.callable->type==CALLABLE_NATIVE_TEXT)){value_free(&module_value);(void)environment_validate_definition(environment,statement->as.import.binding,statement->as.import.binding_length,statement->span,errors);return execution(EXEC_ERROR);}value_free(&module_value);}
            if(!replace_native&&!environment_validate_definition(environment,statement->as.import.binding,statement->as.import.binding_length,statement->span,errors))return execution(EXEC_ERROR);
            {TraceEvent event=trace_event(TRACE_MODULE_IMPORT,statement->span,environment);event.name=statement->as.import.path;event.name_length=statement->as.import.path_length;emit(runtime,event);}
            if(!module_registry_import(runtime->registry,runtime->module==NULL?"./principal.lume":runtime->module->path,statement,&imported,errors))return execution(EXEC_ERROR);
            {TraceEvent event=trace_event(TRACE_MODULE_LOADED,statement->span,environment);event.name=imported->name;event.name_length=strlen(imported->name);emit(runtime,event);}
            module_value=value_module(imported);if(replace_native){if(!environment_set_local_internal(environment,statement->as.import.binding,statement->as.import.binding_length,&module_value))return execution(EXEC_ERROR);}else if(!environment_define(environment,statement->as.import.binding,statement->as.import.binding_length,&module_value,false,statement->span,errors))return execution(EXEC_ERROR);return execution(EXEC_OK);
        }
    }
    (void)fail(errors, statement->span, "Tipo interno de instrucao desconhecido.",
        "Isto indica um erro interno da Lume."); return execution(EXEC_ERROR);
}
static ExecutionResult execute_statements(const StmtArray *statements, Environment *environment,
                                          Runtime *runtime, ErrorList *errors) {
    size_t index;
    for (index = 0U; index < statements->count; index++) {
        const Stmt *statement = statements->data[index];
        if (statement->type == STMT_FUNCTION) {
            Callable *callable;
            Value function_value;
            if (!environment_validate_definition(environment, statement->as.function.name,
                    statement->as.function.name_length, statement->as.function.name_span, errors))
                return execution(EXEC_ERROR);
            callable = callable_new_user(statement, environment);
            if (callable == NULL) return execution(EXEC_ERROR);
            function_value = value_callable(callable);
            if (!environment_define(environment, statement->as.function.name,
                    statement->as.function.name_length, &function_value, false,
                    statement->as.function.name_span, errors)) {
                value_free(&function_value); return execution(EXEC_ERROR);
            }
            value_free(&function_value);
            {TraceEvent event=trace_event(TRACE_DECLARE_FUNCTION,statement->span,environment);event.name=statement->as.function.name;event.name_length=statement->as.function.name_length;emit(runtime,event);}
        }
    }
    for (index = 0U; index < statements->count; index++) {
        if (runtime->trace != NULL && runtime->trace->stop_requested) return execution(EXEC_OK);
        ExecutionResult result = execute_statement(statements->data[index], environment, runtime, errors);
        if (result.status != EXEC_OK) return result;
    }
    return execution(EXEC_OK);
}
static bool install_native(Environment *environment, CallableType type, const char *name,
                           size_t length, size_t arity, ErrorList *errors) {
    SourceSpan span = {{0U, 1U, 1U}, {0U, 1U, 1U}};
    Callable *callable;
    Value value;
    if (environment_has_current(environment, name, length)) return true;
    callable = callable_new_native(type, name, arity);
    if (callable == NULL) return false;
    value = value_callable(callable);
    if (!environment_define_native(environment, name, length, &value, span, errors)) {
        value_free(&value); return false;
    }
    value_free(&value); return true;
}
bool interpreter_execute_program_with_trace(const Program *program, Environment *environment,
    RuntimeIO *io, RuntimeTrace *trace, ErrorList *errors) {
    return interpreter_execute_program_with_modules(program,environment,io,trace,NULL,NULL,errors);
}
bool interpreter_execute_program_with_modules(const Program *program,Environment *environment,
    RuntimeIO *io,RuntimeTrace *trace,ModuleRegistry *registry,LumeModule *module,ErrorList *errors) {
    Runtime runtime; ExecutionResult result;
    if (program == NULL || environment == NULL || errors == NULL || errors->count != 0U)
        return false;
    runtime.io = io; runtime.trace = trace; runtime.call_depth=0U;runtime.registry=registry;runtime.module=module;
    if (io == NULL || io->input == NULL || io->output == NULL) return false;
    if (!install_native(environment, CALLABLE_NATIVE_WRITE, "escreva", 7U, 1U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_READ, "leia", 4U, 0U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_TEXT, "texto", 5U, 1U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_INTEGER, "inteiro", 7U, 1U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_DECIMAL, "decimal", 7U, 1U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_TYPE, "tipo", 4U, 1U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_LENGTH, "tamanho", 7U, 1U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_APPEND, "adicione", 8U, 2U, errors) ||
        !install_native(environment, CALLABLE_NATIVE_REMOVE, "remova", 6U, 2U, errors)) return false;
    {TraceEvent event=trace_event(TRACE_PROGRAM_START,(SourceSpan){{0U,1U,1U},{0U,1U,1U}},environment);emit(&runtime,event);}
    result = execute_statements(&program->statements, environment, &runtime, errors);
    if (trace == NULL || !trace->stop_requested) {TraceEvent event=trace_event(TRACE_PROGRAM_END,(SourceSpan){{0U,1U,1U},{0U,1U,1U}},environment);emit(&runtime,event);}
    value_free(&result.value);
    return result.status == EXEC_OK;
}
bool interpreter_execute_program_with_io(const Program *program, Environment *environment,
                                         RuntimeIO *io, ErrorList *errors) {
    return interpreter_execute_program_with_trace(program, environment, io, NULL, errors);
}
bool interpreter_execute_program(const Program *program, Environment *environment,
                                 ErrorList *errors) {
    RuntimeIO io; runtime_io_default(&io);
    return interpreter_execute_program_with_io(program, environment, &io, errors);
}

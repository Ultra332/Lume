#include "education.h"

#include <inttypes.h>
#include <string.h>
#include "callable.h"
#include "diagnostic.h"
#include "interpreter.h"
#include "lexer.h"
#include "list.h"
#include "module.h"
#include "parser.h"

enum {
    EDUCATION_EVENT_LIMIT = 160,
    EDUCATION_TEXT_LIMIT = 72,
    EDUCATION_LIST_LIMIT = 6,
    EDUCATION_VALUE_DEPTH = 2
};

static void indent(FILE *output, size_t depth) {
    size_t index, amount = depth > 16U ? 16U : depth;
    for (index = 0U; index < amount; index++) fputs("  ", output);
}

static size_t utf8_prefix(const char *bytes, size_t length, size_t limit) {
    size_t at = 0U;
    if (length <= limit) return length;
    while (at < length && at < limit) {
        unsigned char first = (unsigned char)bytes[at];
        size_t width = first < 0x80U ? 1U :
            (first < 0xE0U ? 2U : (first < 0xF0U ? 3U : 4U));
        if (width > limit - at || width > length - at) break;
        at += width;
    }
    return at;
}

static void print_quoted_text(FILE *output, const LumeString *text) {
    size_t index, shown = utf8_prefix(text->bytes, text->length, EDUCATION_TEXT_LIMIT);
    fputc('"', output);
    for (index = 0U; index < shown; index++) {
        char character = text->bytes[index];
        if (character == '"' || character == '\\') {
            fputc('\\', output); fputc(character, output);
        } else if (character == '\n') fputs("\\n", output);
        else if (character == '\r') fputs("\\r", output);
        else if (character == '\t') fputs("\\t", output);
        else fputc(character, output);
    }
    if (shown < text->length) fputs("...", output);
    fputc('"', output);
    if (shown < text->length) fprintf(output, " (%zu bytes)", text->length);
}

static void print_value_depth(FILE *output, const Value *value, size_t depth) {
    size_t index;
    if (value == NULL) { fputs("nulo", output); return; }
    switch (value->type) {
        case VALUE_NULL: fputs("nulo", output); break;
        case VALUE_BOOLEAN: fputs(value->as.boolean ? "verdadeiro" : "falso", output); break;
        case VALUE_INTEGER: fprintf(output, "%" PRId64, value->as.integer); break;
        case VALUE_DECIMAL: fprintf(output, "%.15g", value->as.decimal); break;
        case VALUE_STRING: print_quoted_text(output, &value->as.string); break;
        case VALUE_CALLABLE: fprintf(output, "<funcao %s>", value->as.callable->name); break;
        case VALUE_MODULE: fprintf(output, "<modulo %s>", value->as.module->name); break;
        case VALUE_LIST: {
            const LumeList *list = value->as.list;
            size_t shown = list->count < EDUCATION_LIST_LIMIT ? list->count : EDUCATION_LIST_LIMIT;
            if (depth >= EDUCATION_VALUE_DEPTH) {
                fprintf(output, "[...] (%zu elementos)", list->count); break;
            }
            fputc('[', output);
            for (index = 0U; index < shown; index++) {
                if (index > 0U) fputs(", ", output);
                print_value_depth(output, &list->items[index], depth + 1U);
            }
            if (shown < list->count) fputs(", ...", output);
            fputc(']', output);
            if (shown < list->count) fprintf(output, " (%zu elementos)", list->count);
            break;
        }
    }
}

void education_print_value(FILE *output, const Value *value) {
    if (output != NULL) print_value_depth(output, value, 0U);
}

static const Source *span_source(const EducationRenderer *renderer, SourceSpan span) {
    return span.source != NULL ? span.source : renderer->source;
}

static bool span_bounds(const Source *source, SourceSpan span, size_t *start, size_t *end) {
    if (source == NULL || span.start.offset >= source->length) return false;
    *start = span.start.offset;
    *end = span.end.offset > source->length ? source->length : span.end.offset;
    return *end > *start;
}

static void print_expression(FILE *output, const EducationRenderer *renderer,
                             SourceSpan span) {
    const Source *source = span_source(renderer, span);
    size_t start, end, shown;
    if (!span_bounds(source, span, &start, &end)) return;
    fputc(96, output);
    shown=utf8_prefix(source->bytes+start,end-start,117U);
    fwrite(source->bytes + start, 1U, shown, output);
    if(shown<end-start)fputs("...", output);
    fputc(96, output);
}

static void print_source_line(FILE *output, const EducationRenderer *renderer,
                              SourceSpan span) {
    const Source *source = span_source(renderer, span);
    size_t start, end, shown;
    if (source == NULL || span.start.offset > source->length) return;
    start = span.start.offset;
    while (start > 0U && source->bytes[start - 1U] != '\n') start--;
    end = span.start.offset;
    while (end < source->length && source->bytes[end] != '\n' && source->bytes[end] != '\r') end++;
    if (end <= start) return;
    fputs("    ", output);
    shown=utf8_prefix(source->bytes+start,end-start,117U);
    fwrite(source->bytes + start, 1U, shown, output);
    if(shown<end-start)fputs("...", output);
    fputc('\n', output);
}

static void binding_line(void *context, const char *name, size_t length,
                         const Value *value, bool mutable) {
    FILE *output = context;
    if (value->type == VALUE_CALLABLE && value->as.callable->type != CALLABLE_USER) return;
    fprintf(output, "    %s %.*s = ", mutable ? "variavel" : "constante",
        (int)length, name);
    education_print_value(output, value); fputc('\n', output);
}

void education_show_variables(FILE *output, const Environment *environment) {
    const Environment *scope = environment;
    size_t level = 0U;
    fputs("Variaveis visiveis neste ponto:\n", output);
    while (scope != NULL) {
        fprintf(output, "  Escopo %zu%s:\n", level, scope->is_global ? " (global)" : "");
        environment_visit_current(scope, binding_line, output);
        scope = scope->parent; level++;
    }
}

static void show_stack(EducationRenderer *renderer) {
    size_t index;
    fputs("Pilha de chamadas:\n", renderer->io->output);
    if (renderer->stack_count == 0U)
        fputs("    <programa principal>\n", renderer->io->output);
    for (index = 0U; index < renderer->stack_count; index++)
        fprintf(renderer->io->output, "    %zu. %.*s\n", index + 1U,
            (int)renderer->stack_lengths[index], renderer->stack_names[index]);
}

static void step_prompt(EducationRenderer *renderer) {
    char command[32];
    if (renderer->mode != EDUCATION_STEP || renderer->continuing) return;
    for (;;) {
        fputs("[Enter=proximo, v=variaveis, p=pilha, c=continuar, q=sair] > ",
            renderer->io->output);
        fflush(renderer->io->output);
        if (fgets(command, sizeof(command), renderer->io->input) == NULL) {
            renderer->trace->stop_requested = true; return;
        }
        if (command[0] == '\n' || command[0] == '\r') return;
        if (command[0] == 'v')
            education_show_variables(renderer->io->output, renderer->current_environment);
        else if (command[0] == 'p') show_stack(renderer);
        else if (command[0] == 'c') { renderer->continuing = true; return; }
        else if (command[0] == 'q') { renderer->trace->stop_requested = true; return; }
        else fputs("Comando invalido. Use Enter, v, p, c ou q.\n", renderer->io->output);
    }
}

void education_renderer_init(EducationRenderer *renderer, RuntimeIO *io,
                             const Source *source, RuntimeTrace *trace,
                             EducationMode mode) {
    memset(renderer, 0, sizeof(*renderer));
    renderer->io = io; renderer->source = source; renderer->trace = trace;
    renderer->mode = mode;
}

static bool structural_end(TraceEventType type) {
    return type == TRACE_PROGRAM_END || type == TRACE_WHILE_END ||
        type == TRACE_FOR_END || type == TRACE_FOREACH_END;
}

static bool should_render(EducationRenderer *renderer, const TraceEvent *event) {
    if(event->type>=TRACE_EVENT_COUNT)return false;
    if (event->type == TRACE_IDENTIFIER_READ && event->after != NULL &&
            event->after->type == VALUE_CALLABLE &&
            event->after->as.callable->type != CALLABLE_USER) return false;
    if (renderer->mode == EDUCATION_STEP && !renderer->continuing) return true;
    if (event->type == TRACE_PROGRAM_END) return true;
    if (structural_end(event->type) &&
            renderer->rendered < EDUCATION_EVENT_LIMIT + 8U) return true;
    if (renderer->rendered < EDUCATION_EVENT_LIMIT) return true;
    renderer->omitted++; return false;
}

static const char *binary_operator_name(BinaryOperator operator_type) {
    switch (operator_type) {
        case BINARY_ADD: return "+"; case BINARY_SUBTRACT: return "-";
        case BINARY_MULTIPLY: return "*"; case BINARY_DIVIDE: return "/";
        case BINARY_REMAINDER: return "%"; case BINARY_EQUAL: return "==";
        case BINARY_NOT_EQUAL: return "!="; case BINARY_LESS: return "<";
        case BINARY_LESS_EQUAL: return "<="; case BINARY_GREATER: return ">";
        case BINARY_GREATER_EQUAL: return ">="; case BINARY_LOGICAL_AND: return "e";
        case BINARY_LOGICAL_OR: return "ou";
    }
    return "?";
}

static const char *unary_operator_name(UnaryOperator operator_type) {
    switch (operator_type) {
        case UNARY_POSITIVE: return "+"; case UNARY_NEGATIVE: return "-";
        case UNARY_NOT: return "nao";
    }
    return "?";
}

static void print_arguments(FILE *output, const TraceEvent *event) {
    size_t index;
    if (event->argument_count == 0U) { fputs("    Nenhum argumento.\n", output); return; }
    fputs("    Argumentos:\n", output);
    for (index = 0U; index < event->argument_count; index++) {
        fputs("      ", output); education_print_value(output, &event->arguments[index]);
        fputc('\n', output);
    }
}

static void print_parameters(FILE *output, const TraceEvent *event) {
    const Stmt *function = event->statement;
    size_t index;
    if (function == NULL || function->type != STMT_FUNCTION) return;
    if (function->as.function.parameter_count == 0U) {
        fputs("    A funcao nao possui parametros.\n", output); return;
    }
    fputs("    Parametros neste escopo:\n", output);
    for (index = 0U; index < function->as.function.parameter_count &&
            index < event->argument_count; index++) {
        fprintf(output, "      %.*s = ",
            (int)function->as.function.parameter_lengths[index],
            function->as.function.parameters[index]);
        education_print_value(output, &event->arguments[index]); fputc('\n', output);
    }
}

static void render_header(EducationRenderer *renderer, const TraceEvent *event) {
    FILE *output = renderer->io->output;
    const Source *source = span_source(renderer, event->span);
    renderer->step++; renderer->rendered++;
    if (renderer->mode == EDUCATION_STEP) {
        fprintf(output, "\nPasso %zu", renderer->step);
        if (source != NULL && event->type != TRACE_PROGRAM_START &&
                event->type != TRACE_PROGRAM_END)
            fprintf(output, " — %s:%zu", source->name, event->span.start.line);
        fputc('\n', output);
        if (event->type != TRACE_PROGRAM_START && event->type != TRACE_PROGRAM_END)
            print_source_line(output, renderer, event->span);
        fputs("Acontecimento observado:\n", output);
    } else fprintf(output, "%zu. ", renderer->step);
    indent(output, event->call_depth);
}

static void render_event(EducationRenderer *renderer, const TraceEvent *event) {
    FILE *output = renderer->io->output;
    render_header(renderer, event);
    switch (event->type) {
        case TRACE_PROGRAM_START:
            fputs(renderer->program_depth==1U?"Inicio do programa.":
                "Inicio de uma unidade importada.",output); break;
        case TRACE_PROGRAM_END:
            if (renderer->program_depth==1U && renderer->omitted > 0U)
                fprintf(output, "%zu eventos intermediarios foram resumidos.\n",
                    renderer->omitted);
            fputs(renderer->program_depth==1U?"Fim do programa.":
                "Fim da unidade importada.",output); break;
        case TRACE_DECLARE_VARIABLE:
        case TRACE_DECLARE_CONSTANT:
            fprintf(output, "Criando %s \"%.*s\".\n",
                event->type == TRACE_DECLARE_VARIABLE ? "a variavel" : "a constante",
                (int)event->name_length, event->name);
            indent(output, event->call_depth); fputs("    Expressao: ", output);
            if (event->expression != NULL) print_expression(output, renderer, event->expression->span);
            else fputs("<valor>", output);
            fputc('\n', output); indent(output, event->call_depth);
            fputs("    Valor inicial: ", output); education_print_value(output, event->after);
            fputc('.', output); break;
        case TRACE_DECLARE_FUNCTION:
            fprintf(output, "Preparando a funcao \"%.*s\".",
                (int)event->name_length, event->name); break;
        case TRACE_ASSIGN:
            fprintf(output, "Atualizando \"%.*s\".\n", (int)event->name_length, event->name);
            indent(output, event->call_depth); fputs("    Valor anterior: ", output);
            education_print_value(output, event->before); fputc('\n', output);
            indent(output, event->call_depth); fputs("    Expressao: ", output);
            if (event->expression != NULL) print_expression(output, renderer, event->expression->span);
            else fputs("<valor>", output);
            fputc('\n', output); indent(output, event->call_depth);
            fprintf(output, "    Novo valor de \"%.*s\": ",
                (int)event->name_length, event->name);
            education_print_value(output, event->after); break;
        case TRACE_IDENTIFIER_READ:
            fprintf(output, "Usando \"%.*s\", que atualmente vale ",
                (int)event->name_length, event->name);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_UNARY_EXPRESSION:
            fputs("Avaliando ", output); print_expression(output, renderer, event->span);
            fputs(": ", output);
            fputs(unary_operator_name(event->expression->as.unary.operator_type), output);
            fputc(' ', output); education_print_value(output, event->left);
            fputs(" = ", output); education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_BINARY_EXPRESSION:
            fputs("Avaliando ", output); print_expression(output, renderer, event->span);
            fputs(": ", output); education_print_value(output, event->left);
            fprintf(output, " %s ", binary_operator_name(event->expression->as.binary.operator_type));
            if (event->short_circuit) fputs("<nao avaliado>", output);
            else education_print_value(output, event->right);
            fputs(" = ", output); education_print_value(output, event->after);
            if (event->short_circuit) fputs(" (curto-circuito)", output);
            fputc('.', output); break;
        case TRACE_IF_CONDITION:
            fputs("Verificando a condicao ", output); print_expression(output, renderer, event->span);
            fprintf(output, ". Resultado: %s. ", event->decision ? "verdadeiro" : "falso");
            fputs(event->decision ? "O bloco 'se' sera executado." :
                "O bloco 'se' sera ignorado; o ramo 'senao', se existir, sera usado.", output);
            break;
        case TRACE_WHILE_CONDITION:
            fputs("Verificando a condicao de 'enquanto' ", output);
            print_expression(output, renderer, event->span);
            fprintf(output, ". Resultado: %s.", event->decision ? "verdadeiro" : "falso");
            break;
        case TRACE_WHILE_ITERATION:
            fprintf(output, "Iniciando a iteracao %zu de 'enquanto'.", event->iteration); break;
        case TRACE_WHILE_END:
            fprintf(output, "Laco 'enquanto' encerrado apos %zu iteracao(oes).", event->iteration); break;
        case TRACE_FOR_START:
            fprintf(output, "Iniciando 'para %.*s': valor inicial ",
                (int)event->name_length, event->name);
            education_print_value(output, event->before); fputs(", limite ", output);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_FOR_ITERATION:
            fprintf(output, "Iteracao %zu: %.*s = ", event->iteration,
                (int)event->name_length, event->name);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_FOR_END:
            fprintf(output, "Laco 'para' encerrado apos %zu iteracao(oes).", event->iteration); break;
        case TRACE_FOREACH_START:
            fprintf(output, "Iniciando 'para %.*s em lista' sobre ",
                (int)event->name_length, event->name);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_FOREACH_END:
            fprintf(output, "Laco 'para ... em' encerrado apos %zu iteracao(oes).",
                event->iteration); break;
        case TRACE_BREAK: fputs("'pare' encerra o laco atual.", output); break;
        case TRACE_CONTINUE:
            fputs("'continue' encerra esta iteracao e avanca para a proxima.", output); break;
        case TRACE_FUNCTION_CALL:
            fprintf(output, "Chamando a funcao \"%.*s\".\n",
                (int)event->name_length, event->name);
            print_arguments(output, event); break;
        case TRACE_FUNCTION_ENTER:
            fprintf(output, "Entrando no escopo da funcao \"%.*s\".\n",
                (int)event->name_length, event->name);
            print_parameters(output, event); break;
        case TRACE_FUNCTION_RETURN:
            fprintf(output, "A funcao \"%.*s\" retornou ",
                (int)event->name_length, event->name);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_NATIVE_CALL:
            fprintf(output, "Chamando a funcao nativa \"%.*s\".",
                (int)event->name_length, event->name); break;
        case TRACE_OUTPUT:
            fputs("Saida solicitada por 'escreva': ", output);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_LIST_CREATE:
            fputs("Criando uma lista com ", output);
            fprintf(output, "%zu elemento(s): ", event->after->as.list->count);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_INDEX_READ:
            fprintf(output, "Acessando o indice %" PRId64, event->index);
            fputs("; valor encontrado: ", output); education_print_value(output, event->after);
            fputc('.', output); break;
        case TRACE_INDEX_WRITE:
            fprintf(output, "Alterando o indice %" PRId64 ": ", event->index);
            education_print_value(output, event->before); fputs(" -> ", output);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_LIST_APPEND:
            fputs("Adicionando um elemento; a lista agora e ", output);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_LIST_REMOVE:
            fprintf(output, "Removendo o indice %" PRId64 "; valor obtido: ", event->index);
            education_print_value(output, event->after); fputc('.', output); break;
        case TRACE_MODULE_IMPORT:
            fprintf(output, "Importando o modulo \"%.*s\"",
                (int)event->name_length, event->name);
            if (event->alias != NULL)
                fprintf(output, " como \"%.*s\"", (int)event->alias_length, event->alias);
            fputc('.', output); break;
        case TRACE_MODULE_LOADED:
            fprintf(output, "Modulo \"%.*s\" carregado.",
                (int)event->name_length, event->name); break;
        case TRACE_EVENT_COUNT: break;
    }
    fputc('\n', output);
}

static void stack_push(EducationRenderer *renderer, const TraceEvent *event) {
    size_t length;
    if (renderer->stack_count >= EDUCATION_STACK_LIMIT) return;
    length=utf8_prefix(event->name,event->name_length,EDUCATION_NAME_LIMIT);
    memcpy(renderer->stack_names[renderer->stack_count], event->name, length);
    renderer->stack_names[renderer->stack_count][length] = '\0';
    renderer->stack_lengths[renderer->stack_count] = length;
    renderer->stack_count++;
}

void education_trace_callback(void *context, const TraceEvent *event) {
    EducationRenderer *renderer = context;
    bool pop = event->type == TRACE_FUNCTION_RETURN;
    if(event->type>=TRACE_EVENT_COUNT)return;
    if(event->type==TRACE_PROGRAM_START)renderer->program_depth++;
    renderer->current_environment = event->environment;
    if (event->type == TRACE_FUNCTION_ENTER) stack_push(renderer, event);
    if (should_render(renderer, event)) {
        render_event(renderer, event);
        if(event->type!=TRACE_PROGRAM_END)step_prompt(renderer);
    }
    if (pop && renderer->stack_count > 0U) renderer->stack_count--;
    if(event->type==TRACE_PROGRAM_END&&renderer->program_depth>0U)
        renderer->program_depth--;
    renderer->current_environment = NULL;
}

int education_run_file(const char *path, RuntimeIO io, EducationMode mode) {
    Source source;
    TokenArray tokens;
    ErrorList errors;
    Program *program = NULL;
    Environment environment;
    RuntimeTrace trace;
    EducationRenderer renderer;
    ModuleRegistry registry;
    LumeModule current;
    bool ok;
    source_init(&source); token_array_init(&tokens); error_list_init(&errors);
    environment_init(&environment, NULL);
    ok = source_load_file(&source, path);
    if (ok) ok = lexer_scan(&source, &tokens, &errors);
    if (ok) ok = parser_parse_program(&tokens, &program, &errors);
    trace.callback = education_trace_callback; trace.context = &renderer;
    trace.stop_requested = false;
    education_renderer_init(&renderer, &io, &source, &trace, mode);
    module_registry_init(&registry, &io, &trace);
    memset(&current, 0, sizeof(current)); current.path = (char *)path;
    if (ok) ok = interpreter_execute_program_with_modules(program, &environment,
        &io, &trace, &registry, &current, &errors);
    if (!ok && !trace.stop_requested && renderer.omitted > 0U)
        fprintf(io.output, "%zu eventos intermediarios foram resumidos antes do erro.\n",
            renderer.omitted);
    if (!ok && !trace.stop_requested && errors.count > 0U)
        diagnostic_render(io.output, registry.error_source == NULL ?
            &source : registry.error_source, &errors.data[0]);
    else if (!ok && !trace.stop_requested && errors.count == 0U)
        fprintf(io.output, "Nao foi possivel executar '%s' no modo educacional.\n", path);
    if (trace.stop_requested)
        fputs("Execucao educacional encerrada pelo usuario.\n", io.output);
    environment_free(&environment); module_registry_free(&registry);
    program_free(program); error_list_free(&errors); token_array_free(&tokens);
    source_free(&source);
    return ok || trace.stop_requested ? 0 : 1;
}

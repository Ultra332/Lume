#include "parser.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "memory.h"

typedef struct Parser Parser;
typedef Expr *(*ParseFunction)(Parser *);
#define LUME_MAX_PARSE_DEPTH 128U
struct Parser {
    const TokenArray *tokens;
    ErrorList *errors;
    size_t current;
    size_t grouping_depth;
    bool expression_only;
    bool failed;
    size_t function_depth;
    size_t loop_depth;
    size_t block_depth;
    size_t parse_depth;
};

static void skip_newlines(Parser *parser) {
    while (parser->current < parser->tokens->count &&
           parser->tokens->data[parser->current].type == TOKEN_NEWLINE) parser->current++;
}
static const Token *peek_token(Parser *parser) {
    if (parser->current >= parser->tokens->count) return NULL;
    return &parser->tokens->data[parser->current];
}
static bool match_token(Parser *parser, TokenType type, const Token **matched) {
    const Token *token = peek_token(parser);
    if (token == NULL || token->type != type) return false;
    parser->current++;
    if (matched != NULL) *matched = token;
    return true;
}
static bool match_any(Parser *parser, const TokenType *types, size_t count, const Token **matched) {
    size_t index;
    size_t saved = parser->current;
    if (parser->expression_only || parser->grouping_depth > 0U) skip_newlines(parser);
    const Token *token = peek_token(parser);
    if (token == NULL) return false;
    for (index = 0U; index < count; index++) {
        if (token->type == types[index]) {
            parser->current++;
            if (matched != NULL) *matched = token;
            return true;
        }
    }
    parser->current = saved;
    return false;
}
static SourceSpan fallback_span(Parser *parser) {
    const Token *token = peek_token(parser);
    SourceSpan span = {{0U, 1U, 1U}, {0U, 1U, 1U}, NULL};
    if (token != NULL) span = token->span;
    else if (parser->tokens->count > 0U) span = parser->tokens->data[parser->tokens->count - 1U].span;
    return span;
}
static void parser_error(Parser *parser, SourceSpan span, const char *message, const char *suggestion) {
    LumeError error;
    if (parser->failed) return;
    error.kind = LUME_ERROR_SYNTAX; error.span = span;
    error.message = message; error.suggestion = suggestion;
    error.subject = NULL; error.subject_length = 0U;
    if (!error_list_add(parser->errors, error)) {
        error.kind = LUME_ERROR_MEMORY;
        error.message = "Nao foi possivel reservar memoria para o diagnostico.";
        error.suggestion = "Tente novamente com uma entrada menor.";
        (void)error_list_add(parser->errors, error);
    }
    parser->failed = true;
}
static void parser_memory_error(Parser *parser, SourceSpan span) {
    parser_error(parser, span, "Nao foi possivel reservar memoria para a expressao.",
        "Feche outros programas ou tente uma expressao menor.");
}
static bool parse_integer_value(const Token *token, Value *out) {
    const char *text = token_lexeme(token);
    size_t length = token_length(token), index;
    uint64_t result = 0U;
    for (index = 0U; index < length; index++) {
        unsigned digit = (unsigned)((unsigned char)text[index] - (unsigned char)'0');
        if (digit > 9U || result > ((uint64_t)INT64_MAX - digit) / 10U) return false;
        result = result * 10U + digit;
    }
    *out = value_integer((int64_t)result);
    return true;
}
static bool parse_decimal_value(const Token *token, Value *out) {
    const char *text = token_lexeme(token);
    size_t length = token_length(token), index;
    double result = 0.0;
    double place = 0.1;
    bool after_dot = false;
    for (index = 0U; index < length; index++) {
        unsigned char current = (unsigned char)text[index];
        if (current == (unsigned char)'.') { after_dot = true; continue; }
        if (current < (unsigned char)'0' || current > (unsigned char)'9') return false;
        if (after_dot) { result += (double)(current - (unsigned char)'0') * place; place *= 0.1; }
        else result = result * 10.0 + (double)(current - (unsigned char)'0');
        if (!isfinite(result)) return false;
    }
    *out = value_decimal(result);
    return true;
}

static Expr *parse_or(Parser *parser);
static Expr *parse_nested(Parser *parser) {
    Expr *expression;
    if (parser->parse_depth >= LUME_MAX_PARSE_DEPTH) {
        parser_error(parser, fallback_span(parser),
            "A expressao esta aninhada profundamente demais.",
            "Simplifique a expressao ou divida-a em partes menores.");
        return NULL;
    }
    parser->parse_depth++;
    expression = parse_or(parser);
    parser->parse_depth--;
    return expression;
}
static UnaryOperator unary_operator(TokenType type) {
    if (type == TOKEN_PLUS) return UNARY_POSITIVE;
    if (type == TOKEN_MINUS) return UNARY_NEGATIVE;
    return UNARY_NOT;
}
static BinaryOperator binary_operator(TokenType type) {
    switch (type) {
        case TOKEN_PLUS: return BINARY_ADD;
        case TOKEN_MINUS: return BINARY_SUBTRACT;
        case TOKEN_STAR: return BINARY_MULTIPLY;
        case TOKEN_SLASH: return BINARY_DIVIDE;
        case TOKEN_PERCENT: return BINARY_REMAINDER;
        case TOKEN_EQUAL_EQUAL: return BINARY_EQUAL;
        case TOKEN_BANG_EQUAL: return BINARY_NOT_EQUAL;
        case TOKEN_LESS: return BINARY_LESS;
        case TOKEN_LESS_EQUAL: return BINARY_LESS_EQUAL;
        case TOKEN_GREATER: return BINARY_GREATER;
        case TOKEN_GREATER_EQUAL: return BINARY_GREATER_EQUAL;
        case TOKEN_KW_E: return BINARY_LOGICAL_AND;
        case TOKEN_KW_OU: return BINARY_LOGICAL_OR;
        default: return BINARY_ADD;
    }
}
static Expr *parse_primary(Parser *parser) {
    skip_newlines(parser);
    const Token *token = peek_token(parser);
    Value value;
    Expr *expression;
    if (token == NULL) {
        parser_error(parser, fallback_span(parser), "Era esperada uma expressao.", "Forneca um valor como 10, verdadeiro ou \"texto\".");
        return NULL;
    }
    if (token->type == TOKEN_INTEGER || token->type == TOKEN_DECIMAL || token->type == TOKEN_STRING ||
        token->type == TOKEN_KW_VERDADEIRO || token->type == TOKEN_KW_FALSO || token->type == TOKEN_KW_NULO) {
        parser->current++;
        if (token->type == TOKEN_INTEGER && !parse_integer_value(token, &value)) {
            parser_error(parser, token->span, "Inteiro fora do limite suportado.", "A Lume aceita inteiros de ate 64 bits com sinal.");
            return NULL;
        }
        if (token->type == TOKEN_DECIMAL && !parse_decimal_value(token, &value)) {
            parser_error(parser, token->span, "Decimal fora do limite suportado.", "Use um numero decimal menor.");
            return NULL;
        }
        if (token->type == TOKEN_STRING && !value_string_decode(token_lexeme(token), token_length(token), &value)) {
            parser_memory_error(parser, token->span); return NULL;
        }
        if (token->type == TOKEN_KW_VERDADEIRO) value = value_boolean(true);
        if (token->type == TOKEN_KW_FALSO) value = value_boolean(false);
        if (token->type == TOKEN_KW_NULO) value = value_null();
        expression = expr_new_literal(value, token->span);
        if (expression == NULL) { value_free(&value); parser_memory_error(parser, token->span); }
        return expression;
    }
    if (token->type == TOKEN_LEFT_PAREN) {
        const Token *left = token;
        const Token *right;
        SourceSpan span;
        parser->current++;
        parser->grouping_depth++;
        expression = parse_nested(parser);
        parser->grouping_depth--;
        if (expression == NULL) return NULL;
        skip_newlines(parser);
        if (!match_token(parser, TOKEN_RIGHT_PAREN, &right)) {
            parser_error(parser, fallback_span(parser), "Era esperado ')' para fechar o agrupamento.",
                "Adicione ')' ao final da expressao iniciada com '('.");
            expr_free(expression); return NULL;
        }
        span.start = left->span.start; span.end = right->span.end; span.source = left->span.source;
        {
            Expr *grouping = expr_new_grouping(expression, span);
            if (grouping == NULL) { expr_free(expression); parser_memory_error(parser, span); }
            return grouping;
        }
    }
    if (token->type == TOKEN_LEFT_BRACKET) {
        const Token *left=token,*right; Expr **elements=NULL; size_t count=0U,capacity=0U; SourceSpan span;
        parser->current++; parser->grouping_depth++; skip_newlines(parser);
        if (peek_token(parser)!=NULL && peek_token(parser)->type!=TOKEN_RIGHT_BRACKET) {
            for (;;) { Expr *element=parse_nested(parser); Expr **grown; size_t next;
                if(element==NULL)goto list_error;
                if(count==capacity){if(!memory_grow_capacity(capacity,count+1U,&next)){expr_free(element);goto list_memory;}grown=memory_reallocate_array(elements,next,sizeof(*grown));if(grown==NULL){expr_free(element);goto list_memory;}elements=grown;capacity=next;}
                elements[count++]=element;skip_newlines(parser);if(!match_token(parser,TOKEN_COMMA,NULL))break;skip_newlines(parser);
                if(peek_token(parser)!=NULL&&peek_token(parser)->type==TOKEN_RIGHT_BRACKET){parser_error(parser,peek_token(parser)->span,"Virgula final nao e aceita em listas.","Remova a virgula ou adicione outro elemento.");goto list_error;}
            }
        }
        skip_newlines(parser);if(!match_token(parser,TOKEN_RIGHT_BRACKET,&right)){parser_error(parser,fallback_span(parser),"Era esperado ']' para fechar a lista.","Adicione ']' depois do ultimo elemento.");goto list_error;}
        parser->grouping_depth--;span.start=left->span.start;span.end=right->span.end;span.source=left->span.source;expression=expr_new_list(elements,count,span);if(expression==NULL)goto list_memory_after;return expression;
list_memory: parser_memory_error(parser,left->span);
list_error: parser->grouping_depth--;
list_memory_after: while(count>0U)expr_free(elements[--count]);memory_free(elements);return NULL;
    }
    if (token->type == TOKEN_IDENTIFIER) {
        parser->current++;
        expression = expr_new_identifier(token_lexeme(token), token_length(token), token->span);
        if (expression == NULL) parser_memory_error(parser, token->span);
        return expression;
    }
    parser_error(parser, token->span, "Era esperado um literal ou uma expressao entre parenteses.",
        "Nesta fase, use numeros, textos, booleanos, nulo ou agrupamentos.");
    return NULL;
}
static Expr *parse_call(Parser *parser) {
    Expr *callee = parse_primary(parser);
    if (callee == NULL) return NULL;
    for (;;) {
        const Token *left;
        const Token *right;
        Expr **arguments = NULL;
        size_t count = 0U, capacity = 0U;
        SourceSpan span;
        if (match_token(parser,TOKEN_DOT,&left)) {
            const Token *member; Expr *access;
            if(!match_token(parser,TOKEN_IDENTIFIER,&member)){parser_error(parser,fallback_span(parser),"Era esperado um membro depois de '.'.","Use modulo.nome para acessar um export.");expr_free(callee);return NULL;}
            span.start=callee->span.start;span.end=member->span.end;span.source=callee->span.source;
            access=expr_new_member(callee,token_lexeme(member),token_length(member),member->span,span);
            if(access==NULL){expr_free(callee);parser_memory_error(parser,span);return NULL;}callee=access;continue;
        }
        if (match_token(parser,TOKEN_LEFT_BRACKET,&left)) {
            Expr *index; parser->grouping_depth++; index=parse_nested(parser); skip_newlines(parser);
            if(index==NULL){parser->grouping_depth--;expr_free(callee);return NULL;}
            if(!match_token(parser,TOKEN_RIGHT_BRACKET,&right)){parser->grouping_depth--;parser_error(parser,fallback_span(parser),"Era esperado ']' depois do indice.","Feche a indexacao com ']'.");expr_free(index);expr_free(callee);return NULL;}
            parser->grouping_depth--;span.start=callee->span.start;span.end=right->span.end;span.source=callee->span.source;
            {Expr *indexed=expr_new_index(callee,index,span);if(indexed==NULL){expr_free(index);expr_free(callee);parser_memory_error(parser,span);return NULL;}callee=indexed;}
            continue;
        }
        if (!match_token(parser, TOKEN_LEFT_PAREN, &left)) break;
        parser->grouping_depth++;
        skip_newlines(parser);
        if (peek_token(parser) != NULL && peek_token(parser)->type != TOKEN_RIGHT_PAREN) {
            for (;;) {
                Expr *argument = parse_nested(parser);
                Expr **grown;
                size_t new_capacity;
                if (argument == NULL) goto call_error;
                if (count == capacity) {
                    if (!memory_grow_capacity(capacity, count + 1U, &new_capacity)) {
                        expr_free(argument); parser_memory_error(parser, left->span); goto call_error;
                    }
                    grown = memory_reallocate_array(arguments, new_capacity, sizeof(*grown));
                    if (grown == NULL) { expr_free(argument); parser_memory_error(parser, left->span); goto call_error; }
                    arguments = grown; capacity = new_capacity;
                }
                arguments[count++] = argument;
                skip_newlines(parser);
                if (!match_token(parser, TOKEN_COMMA, NULL)) break;
                skip_newlines(parser);
                if (peek_token(parser) != NULL && peek_token(parser)->type == TOKEN_RIGHT_PAREN) {
                    parser_error(parser, peek_token(parser)->span, "Virgula final nao e aceita nos argumentos.",
                        "Remova a virgula ou adicione outro argumento."); goto call_error;
                }
            }
        }
        skip_newlines(parser);
        if (!match_token(parser, TOKEN_RIGHT_PAREN, &right)) {
            parser_error(parser, fallback_span(parser), "Era esperado ')' depois dos argumentos.",
                "Feche a chamada com ')'."); goto call_error;
        }
        parser->grouping_depth--;
        span.start = callee->span.start; span.end = right->span.end; span.source = callee->span.source;
        {
            Expr *call = expr_new_call(callee, arguments, count, span);
            if (call == NULL) { parser_memory_error(parser, span); goto call_error_without_callee; }
            callee = call;
        }
        continue;
call_error:
        parser->grouping_depth--;
call_error_without_callee:
        while (count > 0U) expr_free(arguments[--count]);
        memory_free(arguments); expr_free(callee); return NULL;
    }
    return callee;
}
static Expr *parse_unary(Parser *parser) {
    static const TokenType operators[] = {TOKEN_MINUS, TOKEN_PLUS, TOKEN_KW_NAO};
    const Token *operator_token;
    if (match_any(parser, operators, sizeof(operators) / sizeof(operators[0]), &operator_token)) {
        Expr *operand;
        Expr *expression;
        if (parser->parse_depth >= LUME_MAX_PARSE_DEPTH) {
            parser_error(parser, operator_token->span,
                "A expressao esta aninhada profundamente demais.",
                "Simplifique a expressao ou divida-a em partes menores.");
            return NULL;
        }
        parser->parse_depth++; operand = parse_unary(parser); parser->parse_depth--;
        if (operand == NULL) return NULL;
        expression = expr_new_unary(unary_operator(operator_token->type), operator_token->span, operand);
        if (expression == NULL) { expr_free(operand); parser_memory_error(parser, operator_token->span); }
        return expression;
    }
    return parse_call(parser);
}
static Expr *parse_binary(Parser *parser, ParseFunction lower,
                          const TokenType *operators, size_t operator_count) {
    Expr *left = lower(parser);
    const Token *operator_token;
    if (left == NULL) return NULL;
    while (match_any(parser, operators, operator_count, &operator_token)) {
        Expr *right = lower(parser);
        Expr *combined;
        if (right == NULL) { expr_free(left); return NULL; }
        combined = expr_new_binary(left, binary_operator(operator_token->type), operator_token->span, right);
        if (combined == NULL) {
            expr_free(left); expr_free(right); parser_memory_error(parser, operator_token->span); return NULL;
        }
        left = combined;
    }
    return left;
}
static Expr *parse_factor(Parser *parser) {
    static const TokenType ops[] = {TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT};
    return parse_binary(parser, parse_unary, ops, sizeof(ops) / sizeof(ops[0]));
}
static Expr *parse_term(Parser *parser) {
    static const TokenType ops[] = {TOKEN_PLUS, TOKEN_MINUS};
    return parse_binary(parser, parse_factor, ops, sizeof(ops) / sizeof(ops[0]));
}
static Expr *parse_comparison(Parser *parser) {
    static const TokenType ops[] = {TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL};
    return parse_binary(parser, parse_term, ops, sizeof(ops) / sizeof(ops[0]));
}
static Expr *parse_equality(Parser *parser) {
    static const TokenType ops[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    return parse_binary(parser, parse_comparison, ops, sizeof(ops) / sizeof(ops[0]));
}
static Expr *parse_and(Parser *parser) {
    static const TokenType ops[] = {TOKEN_KW_E};
    return parse_binary(parser, parse_equality, ops, sizeof(ops) / sizeof(ops[0]));
}
static Expr *parse_or(Parser *parser) {
    static const TokenType ops[] = {TOKEN_KW_OU};
    return parse_binary(parser, parse_and, ops, sizeof(ops) / sizeof(ops[0]));
}

bool parser_parse_expression(const TokenArray *tokens, Expr **out_expr, ErrorList *errors) {
    Parser parser;
    Expr *expression;
    const Token *remaining;
    if (tokens == NULL || out_expr == NULL || errors == NULL || errors->count != 0U) return false;
    *out_expr = NULL;
    parser.tokens = tokens; parser.errors = errors; parser.current = 0U;
    parser.grouping_depth = 0U; parser.expression_only = true;
    parser.failed = false; parser.function_depth = 0U; parser.block_depth = 0U; parser.parse_depth = 0U;
    skip_newlines(&parser);
    expression = parse_or(&parser);
    if (expression == NULL) return false;
    skip_newlines(&parser);
    remaining = peek_token(&parser);
    if (remaining == NULL || remaining->type != TOKEN_EOF) {
        parser_error(&parser, fallback_span(&parser), "Ha tokens extras depois da expressao.",
            "O modo de expressao aceita exatamente uma expressao.");
        expr_free(expression); return false;
    }
    *out_expr = expression;
    return true;
}

static bool is_terminator(TokenType type) {
    return type == TOKEN_NEWLINE || type == TOKEN_SEMICOLON;
}
static size_t consume_terminators(Parser *parser) {
    size_t count = 0U;
    const Token *token = peek_token(parser);
    while (token != NULL && is_terminator(token->type)) {
        parser->current++; count++; token = peek_token(parser);
    }
    return count;
}
static bool require_token(Parser *parser, TokenType type, const Token **out,
                          const char *message, const char *suggestion) {
    if (match_token(parser, type, out)) return true;
    parser_error(parser, fallback_span(parser), message, suggestion);
    return false;
}
static Stmt *parse_statement(Parser *parser);
static Stmt *parse_block(Parser *parser, const Token *left_brace);
static bool finish_statement(Parser *parser, TokenType closing_type) {
    const Token *next = peek_token(parser);
    if (next == NULL || next->type == TOKEN_EOF || next->type == closing_type) return true;
    if (consume_terminators(parser) > 0U) return true;
    parser_error(parser, next->span, "Era esperada uma quebra de linha ou ';' depois da instrucao.",
        "Separe as instrucoes com uma nova linha ou ponto e virgula.");
    return false;
}
static Stmt *parse_declaration(Parser *parser, bool mutable, const Token *keyword) {
    const Token *name;
    const Token *equal;
    Expr *initializer;
    Stmt *statement;
    if (!require_token(parser, TOKEN_IDENTIFIER, &name,
            "Era esperado um nome depois da declaracao.",
            "Use, por exemplo: variavel idade = 18.")) return NULL;
    if (!require_token(parser, TOKEN_EQUAL, &equal,
            "Era esperado '=' e um inicializador depois do nome.",
            "Declaracoes exigem um valor inicial, como: variavel idade = 18.")) return NULL;
    (void)equal;
    initializer = parse_or(parser);
    if (initializer == NULL) return NULL;
    statement = stmt_new_declaration(mutable, token_lexeme(name), token_length(name),
        name->span, initializer);
    if (statement == NULL) {
        expr_free(initializer); parser_memory_error(parser, keyword->span); return NULL;
    }
    statement->span.start = keyword->span.start;
    statement->span.source = keyword->span.source;
    return statement;
}
static Stmt *parse_required_block(Parser *parser, const char *context) {
    const Token *left_brace;
    skip_newlines(parser);
    if (!match_token(parser, TOKEN_LEFT_BRACE, &left_brace)) {
        parser_error(parser, fallback_span(parser), "Era esperado um bloco entre '{' e '}'.", context);
        return NULL;
    }
    return parse_block(parser, left_brace);
}
static Stmt *parse_if_statement(Parser *parser, const Token *keyword) {
    Expr *condition = parse_or(parser);
    Stmt *then_branch;
    Stmt *else_branch = NULL;
    Stmt *statement;
    SourceSpan span;
    size_t before_else;
    const Token *else_token;
    const Token *nested_if;
    if (condition == NULL) return NULL;
    then_branch = parse_required_block(parser, "Use: se condicao { instrucoes }.");
    if (then_branch == NULL) { expr_free(condition); return NULL; }
    before_else = parser->current;
    skip_newlines(parser);
    if (match_token(parser, TOKEN_KW_SENAO, &else_token)) {
        (void)else_token;
        if (match_token(parser, TOKEN_KW_SE, &nested_if)) {
            else_branch = parse_if_statement(parser, nested_if);
        } else {
            else_branch = parse_required_block(parser, "Use: senao { instrucoes }.");
        }
        if (else_branch == NULL) {
            expr_free(condition); stmt_free(then_branch); return NULL;
        }
    } else {
        parser->current = before_else;
    }
    span.start = keyword->span.start;
    span.end = else_branch != NULL ? else_branch->span.end : then_branch->span.end;
    span.source = keyword->span.source;
    statement = stmt_new_if(condition, then_branch, else_branch, span);
    if (statement == NULL) {
        expr_free(condition); stmt_free(then_branch); stmt_free(else_branch);
        parser_memory_error(parser, span);
    }
    return statement;
}
static Stmt *parse_while_statement(Parser *parser, const Token *keyword) {
    Expr *condition = parse_or(parser);
    Stmt *body;
    Stmt *statement;
    SourceSpan span;
    if (condition == NULL) return NULL;
    parser->loop_depth++;
    body = parse_required_block(parser, "Use: enquanto condicao { instrucoes }.");
    parser->loop_depth--;
    if (body == NULL) { expr_free(condition); return NULL; }
    span.start = keyword->span.start; span.end = body->span.end; span.source = keyword->span.source;
    statement = stmt_new_while(condition, body, span);
    if (statement == NULL) {
        expr_free(condition); stmt_free(body); parser_memory_error(parser, span);
    }
    return statement;
}
static Stmt *parse_for_statement(Parser *parser, const Token *keyword) {
    const Token *name;
    const Token *ignored;
    Expr *start;
    Expr *end;
    Stmt *body;
    Stmt *statement;
    SourceSpan span;
    if (!require_token(parser, TOKEN_IDENTIFIER, &name,
            "Era esperado o nome do iterador depois de 'para'.",
            "Use: para i de 1 ate 5 { instrucoes }.")) return NULL;
    if (match_token(parser, TOKEN_KW_EM, &ignored)) {
        Expr *iterable = parse_or(parser);
        if (iterable == NULL) return NULL;
        parser->loop_depth++;
        body = parse_required_block(parser, "Use: para item em lista { instrucoes }.");
        parser->loop_depth--;
        if (body == NULL) { expr_free(iterable); return NULL; }
        span.start = keyword->span.start; span.end = body->span.end; span.source = keyword->span.source;
        statement = stmt_new_for_each(token_lexeme(name), token_length(name), name->span,
            iterable, body, span);
        if (statement == NULL) {
            expr_free(iterable); stmt_free(body); parser_memory_error(parser, span);
        }
        return statement;
    }
    if (!require_token(parser, TOKEN_KW_DE, &ignored,
            "Era esperada a palavra 'de' ou 'em' depois do iterador.",
            "Use: para i de 1 ate 5 { ... } ou para item em lista { ... }.")) return NULL;
    start = parse_or(parser);
    if (start == NULL) return NULL;
    if (!require_token(parser, TOKEN_KW_ATE, &ignored,
            "Era esperada a palavra 'ate' depois do limite inicial.",
            "Separe os limites assim: de inicio ate fim.")) {
        expr_free(start); return NULL;
    }
    end = parse_or(parser);
    if (end == NULL) { expr_free(start); return NULL; }
    parser->loop_depth++;
    body = parse_required_block(parser, "Use: para i de inicio ate fim { instrucoes }.");
    parser->loop_depth--;
    if (body == NULL) { expr_free(start); expr_free(end); return NULL; }
    span.start = keyword->span.start; span.end = body->span.end; span.source = keyword->span.source;
    statement = stmt_new_for(token_lexeme(name), token_length(name), name->span,
        start, end, body, span);
    if (statement == NULL) {
        expr_free(start); expr_free(end); stmt_free(body); parser_memory_error(parser, span);
    }
    return statement;
}
static void free_parameters(char **names, size_t *lengths, SourceSpan *spans, size_t count) {
    while (count > 0U) memory_free(names[--count]);
    memory_free(names); memory_free(lengths); memory_free(spans);
}
static Stmt *parse_function(Parser *parser, const Token *keyword) {
    const Token *name, *left, *right;
    char **names = NULL;
    size_t *lengths = NULL, count = 0U, capacity = 0U;
    SourceSpan *spans = NULL;
    Stmt *body, *statement;
    SourceSpan span;
    if (!require_token(parser, TOKEN_IDENTIFIER, &name, "Era esperado o nome da funcao.",
            "Use: funcao nome(parametros) { instrucoes }.")) return NULL;
    if (!require_token(parser, TOKEN_LEFT_PAREN, &left, "Era esperado '(' depois do nome da funcao.",
            "Adicione a lista de parametros entre parenteses.")) return NULL;
    (void)left; skip_newlines(parser);
    if (peek_token(parser) != NULL && peek_token(parser)->type != TOKEN_RIGHT_PAREN) {
        for (;;) {
            const Token *parameter;
            size_t index, new_capacity;
            char **grown_names; size_t *grown_lengths; SourceSpan *grown_spans;
            if (!require_token(parser, TOKEN_IDENTIFIER, &parameter, "Era esperado um nome de parametro.",
                    "Separe parametros por virgulas.")) goto parameter_error;
            for (index = 0U; index < count; index++) {
                if (lengths[index] == token_length(parameter) &&
                    memcmp(names[index], token_lexeme(parameter), lengths[index]) == 0) {
                    parser_error(parser, parameter->span, "Parametro duplicado na mesma funcao.",
                        "Use nomes diferentes para cada parametro."); goto parameter_error;
                }
            }
            if (count == capacity) {
                if (!memory_grow_capacity(capacity, count + 1U, &new_capacity)) goto parameter_memory;
                grown_names = memory_reallocate_array(names, new_capacity, sizeof(*grown_names));
                if (grown_names == NULL) goto parameter_memory;
                names = grown_names;
                grown_lengths = memory_reallocate_array(lengths, new_capacity, sizeof(*grown_lengths));
                if (grown_lengths == NULL) goto parameter_memory;
                lengths = grown_lengths;
                grown_spans = memory_reallocate_array(spans, new_capacity, sizeof(*grown_spans));
                if (grown_spans == NULL) goto parameter_memory;
                spans = grown_spans; capacity = new_capacity;
            }
            names[count] = memory_copy_string(token_lexeme(parameter), token_length(parameter));
            if (names[count] == NULL) goto parameter_memory;
            lengths[count] = token_length(parameter); spans[count] = parameter->span; count++;
            skip_newlines(parser);
            if (!match_token(parser, TOKEN_COMMA, NULL)) break;
            skip_newlines(parser);
        }
    }
    if (!require_token(parser, TOKEN_RIGHT_PAREN, &right, "Era esperado ')' depois dos parametros.",
            "Feche a lista de parametros com ')'.")) goto parameter_error;
    (void)right; skip_newlines(parser);
    if (!match_token(parser, TOKEN_LEFT_BRACE, &left)) {
        parser_error(parser, fallback_span(parser), "Era esperado o corpo da funcao entre '{' e '}'.",
            "Adicione um bloco depois da assinatura."); goto parameter_error;
    }
    {
    size_t outer_loop_depth = parser->loop_depth;
    parser->loop_depth = 0U;
    parser->function_depth++;
    body = parse_block(parser, left);
    parser->function_depth--;
    parser->loop_depth = outer_loop_depth;
    }
    if (body == NULL) goto parameter_error;
    span.start = keyword->span.start; span.end = body->span.end; span.source = keyword->span.source;
    statement = stmt_new_function(token_lexeme(name), token_length(name), name->span,
        names, lengths, spans, count, body, span);
    if (statement == NULL) { stmt_free(body); parser_memory_error(parser, span); goto parameter_error; }
    return statement;
parameter_memory:
    parser_memory_error(parser, name->span);
parameter_error:
    free_parameters(names, lengths, spans, count); return NULL;
}
static Stmt *parse_return(Parser *parser, const Token *keyword) {
    Expr *value = NULL;
    SourceSpan span = keyword->span;
    const Token *next;
    if (parser->function_depth == 0U) {
        parser_error(parser, keyword->span, "'retorne' so pode ser usado dentro de uma funcao.",
            "Remova o retorno ou coloque-o no corpo de uma funcao."); return NULL;
    }
    next = peek_token(parser);
    if (next != NULL && !is_terminator(next->type) && next->type != TOKEN_RIGHT_BRACE && next->type != TOKEN_EOF) {
        value = parse_or(parser);
        if (value == NULL) return NULL;
        span.end = value->span.end;
    }
    {
        Stmt *statement = stmt_new_return(value, span);
        if (statement == NULL) { expr_free(value); parser_memory_error(parser, span); }
        return statement;
    }
}
static Stmt *parse_loop_control(Parser *parser, const Token *keyword, bool is_break) {
    Stmt *statement;
    if (parser->loop_depth == 0U) {
        parser_error(parser, keyword->span,
            is_break ? "'pare' so pode ser usado dentro de um laco." : "'continue' so pode ser usado dentro de um laco.",
            is_break ? "Coloque 'pare' dentro de 'enquanto' ou 'para'." : "Coloque 'continue' dentro de 'enquanto' ou 'para'.");
        return NULL;
    }
    statement = stmt_new_loop_control(is_break, keyword->span);
    if (statement == NULL) parser_memory_error(parser, keyword->span);
    return statement;
}
static Stmt *parse_import(Parser *parser,const Token *keyword){const Token *path;Value decoded=value_null();Stmt *statement;SourceSpan span;if(parser->block_depth!=0U||parser->function_depth!=0U){parser_error(parser,keyword->span,"'importe' so pode ser usado no nivel principal do modulo.","Mova o import para o inicio do arquivo.");return NULL;}if(!require_token(parser,TOKEN_STRING,&path,"Era esperado um caminho de modulo depois de 'importe'.","Use: importe \"matematica\"."))return NULL;if(!value_string_decode(token_lexeme(path),token_length(path),&decoded)){parser_memory_error(parser,path->span);return NULL;}span.start=keyword->span.start;span.end=path->span.end;span.source=keyword->span.source;statement=stmt_new_import(decoded.as.string.bytes,decoded.as.string.length,path->span,span);value_free(&decoded);if(statement==NULL)parser_memory_error(parser,span);return statement;}
static Stmt *parse_block(Parser *parser, const Token *left_brace) {
    StmtArray statements = {NULL, 0U, 0U};
    const Token *right_brace;
    parser->block_depth++;consume_terminators(parser);
    while (peek_token(parser) != NULL && peek_token(parser)->type != TOKEN_RIGHT_BRACE &&
           peek_token(parser)->type != TOKEN_EOF) {
        Stmt *statement = parse_statement(parser);
        if (statement == NULL) { parser->block_depth--;stmt_array_free(&statements); return NULL; }
        if (!stmt_array_add(&statements, statement)) {
            stmt_free(statement); stmt_array_free(&statements);
            parser_memory_error(parser, left_brace->span); return NULL;
        }
        if (!finish_statement(parser, TOKEN_RIGHT_BRACE)) {
            parser->block_depth--;stmt_array_free(&statements); return NULL;
        }
        consume_terminators(parser);
    }
    if (!require_token(parser, TOKEN_RIGHT_BRACE, &right_brace,
            "Era esperado '}' para fechar o bloco.",
            "Adicione '}' depois da ultima instrucao do bloco.")) {
        parser->block_depth--;stmt_array_free(&statements); return NULL;
    }
    {
        SourceSpan span = {left_brace->span.start, right_brace->span.end, left_brace->span.source};
        Stmt *block;parser->block_depth--;block = stmt_new_block(statements, span);
        if (block == NULL) { stmt_array_free(&statements); parser_memory_error(parser, span); }
        return block;
    }
}
static Stmt *parse_statement(Parser *parser) {
    const Token *token = peek_token(parser);
    if (token == NULL) return NULL;
    if(token->type==TOKEN_KW_IMPORTE){parser->current++;return parse_import(parser,token);}
    if(token->type==TOKEN_KW_EXPORTE){Stmt *statement;if(parser->block_depth!=0U){parser_error(parser,token->span,"'exporte' so pode modificar declaracoes no nivel do modulo.","Mova a declaracao exportada para o nivel principal.");return NULL;}parser->current++;token=peek_token(parser);if(token==NULL||(token->type!=TOKEN_KW_VARIAVEL&&token->type!=TOKEN_KW_CONSTANTE&&token->type!=TOKEN_KW_FUNCAO)){parser_error(parser,fallback_span(parser),"'exporte' deve preceder variavel, constante ou funcao.","Use: exporte funcao nome() { }.");return NULL;}parser->current++;if(token->type==TOKEN_KW_FUNCAO)statement=parse_function(parser,token);else statement=parse_declaration(parser,token->type==TOKEN_KW_VARIAVEL,token);if(statement!=NULL)statement->exported=true;return statement;}
    if (token->type == TOKEN_KW_VARIAVEL || token->type == TOKEN_KW_CONSTANTE) {
        parser->current++;
        return parse_declaration(parser, token->type == TOKEN_KW_VARIAVEL, token);
    }
    if (token->type == TOKEN_KW_FUNCAO) { parser->current++; return parse_function(parser, token); }
    if (token->type == TOKEN_KW_RETORNE) { parser->current++; return parse_return(parser, token); }
    if (token->type == TOKEN_KW_PARE) { parser->current++; return parse_loop_control(parser, token, true); }
    if (token->type == TOKEN_KW_CONTINUE) { parser->current++; return parse_loop_control(parser, token, false); }
    if (token->type == TOKEN_KW_SE) {
        parser->current++;
        return parse_if_statement(parser, token);
    }
    if (token->type == TOKEN_KW_ENQUANTO) {
        parser->current++;
        return parse_while_statement(parser, token);
    }
    if (token->type == TOKEN_KW_PARA) {
        parser->current++;
        return parse_for_statement(parser, token);
    }
    if (token->type == TOKEN_KW_SENAO) {
        parser_error(parser, token->span, "'senao' apareceu sem um 'se' correspondente.",
            "Use 'senao' imediatamente depois do bloco de um 'se'.");
        return NULL;
    }
    if (token->type == TOKEN_LEFT_BRACE) {
        parser->current++;
        return parse_block(parser, token);
    }
    {
        Expr *expression = parse_or(parser);
        Stmt *statement;
        SourceSpan span;
        if (expression == NULL) return NULL;
        if (match_token(parser,TOKEN_EQUAL,NULL)) {
            Expr *value=parse_or(parser); if(value==NULL){expr_free(expression);return NULL;}
            span.start=expression->span.start;span.end=value->span.end;span.source=expression->span.source;
            if(expression->type==EXPR_IDENTIFIER){statement=stmt_new_assignment(expression->as.identifier.name,expression->as.identifier.length,expression->span,value);expr_free(expression);if(statement==NULL){expr_free(value);parser_memory_error(parser,span);}return statement;}
            if(expression->type==EXPR_INDEX){Expr *target=expression->as.index.target,*index=expression->as.index.index;expression->as.index.target=NULL;expression->as.index.index=NULL;expr_free(expression);statement=stmt_new_index_assignment(target,index,value,span);if(statement==NULL){expr_free(target);expr_free(index);expr_free(value);parser_memory_error(parser,span);}return statement;}
            parser_error(parser,expression->span,"Alvo invalido para atribuicao.","Atribua a uma variavel ou a um indice de lista.");expr_free(expression);expr_free(value);return NULL;
        }
        span = expression->span;
        statement = stmt_new_expression(expression);
        if (statement == NULL) { expr_free(expression); parser_memory_error(parser, span); }
        return statement;
    }
}
bool parser_parse_program(const TokenArray *tokens, Program **out_program, ErrorList *errors) {
    Parser parser;
    Program *program;
    const Token *next;
    if (tokens == NULL || out_program == NULL || errors == NULL || errors->count != 0U) return false;
    *out_program = NULL;
    parser.tokens = tokens; parser.errors = errors; parser.current = 0U;
    parser.grouping_depth = 0U; parser.expression_only = false;
    parser.failed = false; parser.function_depth = 0U; parser.loop_depth = 0U;
    parser.block_depth=0U; parser.parse_depth=0U;
    program = program_new();
    if (program == NULL) {
        parser_memory_error(&parser, fallback_span(&parser)); return false;
    }
    consume_terminators(&parser);
    next = peek_token(&parser);
    while (next != NULL && next->type != TOKEN_EOF) {
        Stmt *statement = parse_statement(&parser);
        if (statement == NULL) { program_free(program); return false; }
        if (!program_add_statement(program, statement)) {
            SourceSpan span = statement->span;
            stmt_free(statement); parser_memory_error(&parser, span);
            program_free(program); return false;
        }
        if (!finish_statement(&parser, TOKEN_EOF)) { program_free(program); return false; }
        consume_terminators(&parser);
        next = peek_token(&parser);
    }
    if (next == NULL) {
        parser_error(&parser, fallback_span(&parser), "O fluxo de tokens nao possui EOF.",
            "Isto indica um erro interno entre lexer e parser.");
        program_free(program); return false;
    }
    *out_program = program;
    return true;
}

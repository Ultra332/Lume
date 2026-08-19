#include "lexer.h"

#include <string.h>

typedef struct {
    const Source *source;
    TokenArray *tokens;
    ErrorList *errors;
    size_t start;
    size_t current;
    size_t start_line;
    size_t start_column;
    size_t line;
    size_t column;
} Lexer;

static bool at_end(const Lexer *lexer) { return lexer->current >= lexer->source->length; }
static unsigned char peek(const Lexer *lexer) {
    return at_end(lexer) ? 0U : (unsigned char)lexer->source->bytes[lexer->current];
}
static unsigned char peek_next(const Lexer *lexer) {
    return lexer->current + 1U >= lexer->source->length ? 0U :
        (unsigned char)lexer->source->bytes[lexer->current + 1U];
}
static unsigned char advance(Lexer *lexer) {
    unsigned char value = peek(lexer);
    if (!at_end(lexer)) { lexer->current++; lexer->column++; }
    return value;
}
static bool match(Lexer *lexer, unsigned char expected) {
    if (peek(lexer) != expected || at_end(lexer)) return false;
    (void)advance(lexer); return true;
}
static SourceLocation location(size_t offset, size_t line, size_t column) {
    SourceLocation result = {offset, line, column}; return result;
}
static SourceSpan current_span(const Lexer *lexer) {
    SourceSpan span;
    span.start = location(lexer->start, lexer->start_line, lexer->start_column);
    span.end = location(lexer->current, lexer->line, lexer->column);
    span.source = lexer->source;
    return span;
}
static bool report_error(Lexer *lexer, const char *message, const char *suggestion) {
    LumeError error;
    error.kind = LUME_ERROR_LEXICAL;
    error.span = current_span(lexer);
    error.message = message;
    error.suggestion = suggestion;
    error.subject = NULL; error.subject_length = 0U;
    return error_list_add(lexer->errors, error);
}
static bool report_memory_error(Lexer *lexer) {
    LumeError error;
    error.kind = LUME_ERROR_MEMORY;
    error.span = current_span(lexer);
    error.message = "Nao foi possivel reservar memoria para os tokens.";
    error.suggestion = "Feche outros programas ou tente uma entrada menor.";
    error.subject = NULL; error.subject_length = 0U;
    (void)error_list_add(lexer->errors, error);
    return false;
}
static bool add_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.source = lexer->source;
    token.span = current_span(lexer);
    return token_array_add(lexer->tokens, token) ? true : report_memory_error(lexer);
}
static bool is_alpha(unsigned char value) {
    return (value >= (unsigned char)'a' && value <= (unsigned char)'z') ||
        (value >= (unsigned char)'A' && value <= (unsigned char)'Z') || value == (unsigned char)'_';
}
static bool is_digit(unsigned char value) {
    return value >= (unsigned char)'0' && value <= (unsigned char)'9';
}
static bool is_alphanumeric(unsigned char value) { return is_alpha(value) || is_digit(value); }
static bool lexeme_equals(const Lexer *lexer, const char *word) {
    size_t length = lexer->current - lexer->start;
    return strlen(word) == length && memcmp(lexer->source->bytes + lexer->start, word, length) == 0;
}
static TokenType identifier_type(const Lexer *lexer) {
    static const struct { const char *word; TokenType type; } keywords[] = {
        {"variavel", TOKEN_KW_VARIAVEL}, {"constante", TOKEN_KW_CONSTANTE},
        {"se", TOKEN_KW_SE}, {"senao", TOKEN_KW_SENAO},
        {"enquanto", TOKEN_KW_ENQUANTO}, {"para", TOKEN_KW_PARA},
        {"de", TOKEN_KW_DE}, {"ate", TOKEN_KW_ATE}, {"funcao", TOKEN_KW_FUNCAO},
        {"retorne", TOKEN_KW_RETORNE}, {"verdadeiro", TOKEN_KW_VERDADEIRO},
        {"falso", TOKEN_KW_FALSO}, {"nulo", TOKEN_KW_NULO}, {"e", TOKEN_KW_E},
        {"ou", TOKEN_KW_OU}, {"nao", TOKEN_KW_NAO},
        {"importe", TOKEN_KW_IMPORTE}, {"exporte", TOKEN_KW_EXPORTE}
    };
    size_t index;
    for (index = 0U; index < sizeof(keywords) / sizeof(keywords[0]); index++) {
        if (lexeme_equals(lexer, keywords[index].word)) return keywords[index].type;
    }
    return TOKEN_IDENTIFIER;
}
static bool scan_identifier(Lexer *lexer) {
    while (is_alphanumeric(peek(lexer))) (void)advance(lexer);
    return add_token(lexer, identifier_type(lexer));
}
static bool invalid_number_suffix(Lexer *lexer) {
    unsigned char next = peek(lexer);
    if (is_alpha(next) || next >= 0x80U) {
        while (is_alphanumeric(peek(lexer)) || peek(lexer) >= 0x80U) (void)advance(lexer);
        (void)report_error(lexer, "Numero seguido por caracteres de identificador.",
            "Separe o numero do nome com espaco ou use um identificador que comece por letra ou _." );
        return true;
    }
    return false;
}
static bool scan_number(Lexer *lexer) {
    bool decimal = false;
    while (is_digit(peek(lexer))) (void)advance(lexer);
    if (peek(lexer) == (unsigned char)'.') {
        if (!is_digit(peek_next(lexer))) {
            (void)advance(lexer);
            (void)report_error(lexer, "Decimal incompleto.",
                "Use digitos dos dois lados do ponto, por exemplo: 1.0." );
            return false;
        }
        decimal = true;
        (void)advance(lexer);
        while (is_digit(peek(lexer))) (void)advance(lexer);
    }
    if (invalid_number_suffix(lexer)) return false;
    return add_token(lexer, decimal ? TOKEN_DECIMAL : TOKEN_INTEGER);
}
static bool valid_escape(unsigned char value) {
    return value == (unsigned char)'\\' || value == (unsigned char)'"' ||
        value == (unsigned char)'n' || value == (unsigned char)'r' || value == (unsigned char)'t';
}
static bool scan_string(Lexer *lexer) {
    while (!at_end(lexer) && peek(lexer) != (unsigned char)'"' &&
           peek(lexer) != (unsigned char)'\n' && peek(lexer) != (unsigned char)'\r') {
        if (peek(lexer) == (unsigned char)'\\') {
            (void)advance(lexer);
            if (at_end(lexer)) break;
            if (!valid_escape(peek(lexer))) {
                (void)advance(lexer);
                (void)report_error(lexer, "Sequencia de escape desconhecida.",
                    "Use apenas \\n, \\r, \\t, \\\" ou \\\\." );
                return false;
            }
        }
        (void)advance(lexer);
    }
    if (at_end(lexer) || peek(lexer) == (unsigned char)'\n' || peek(lexer) == (unsigned char)'\r') {
        (void)report_error(lexer, "String nao terminada.",
            "Uma string iniciada com aspas precisa ser fechada na mesma linha." );
        return false;
    }
    (void)advance(lexer);
    return add_token(lexer, TOKEN_STRING);
}
static bool scan_one(Lexer *lexer) {
    unsigned char value = advance(lexer);
    switch (value) {
        case ' ': case '\t': case '\r': return true;
        case '\n':
            if (!add_token(lexer, TOKEN_NEWLINE)) return false;
            lexer->line++; lexer->column = 1U; return true;
        case '(': return add_token(lexer, TOKEN_LEFT_PAREN);
        case ')': return add_token(lexer, TOKEN_RIGHT_PAREN);
        case '{': return add_token(lexer, TOKEN_LEFT_BRACE);
        case '}': return add_token(lexer, TOKEN_RIGHT_BRACE);
        case '[': return add_token(lexer, TOKEN_LEFT_BRACKET);
        case ']': return add_token(lexer, TOKEN_RIGHT_BRACKET);
        case ',': return add_token(lexer, TOKEN_COMMA);
        case '.':
            if(is_digit(peek(lexer))){while(is_digit(peek(lexer)))(void)advance(lexer);(void)report_error(lexer,"Decimal sem digito antes do ponto.","Use zero antes do ponto, como 0.5.");return false;}
            return add_token(lexer, TOKEN_DOT);
        case ':': return add_token(lexer, TOKEN_COLON);
        case ';': return add_token(lexer, TOKEN_SEMICOLON);
        case '+': return add_token(lexer, TOKEN_PLUS);
        case '-': return add_token(lexer, TOKEN_MINUS);
        case '*': return add_token(lexer, TOKEN_STAR);
        case '%': return add_token(lexer, TOKEN_PERCENT);
        case '=': return add_token(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return add_token(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>': return add_token(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '!':
            if (match(lexer, '=')) return add_token(lexer, TOKEN_BANG_EQUAL);
            (void)report_error(lexer, "Operador '!' incompleto.", "Use '!=' para diferenca ou 'nao' para negacao." );
            return false;
        case '/':
            if (match(lexer, '/')) {
                while (!at_end(lexer) && peek(lexer) != (unsigned char)'\n') (void)advance(lexer);
                return true;
            }
            return add_token(lexer, TOKEN_SLASH);
        case '"': return scan_string(lexer);
        default:
            if (is_alpha(value)) return scan_identifier(lexer);
            if (is_digit(value)) return scan_number(lexer);
            if (value >= 0x80U) {
                while (peek(lexer) >= 0x80U || is_alphanumeric(peek(lexer))) (void)advance(lexer);
                (void)report_error(lexer, "Caractere nao permitido em identificador na Lume 0.1.",
                    "Identificadores aceitam apenas A-Z, a-z, 0-9 e _." );
            } else {
                (void)report_error(lexer, "Caractere nao reconhecido.",
                    "Remova o caractere ou consulte os simbolos aceitos pela Lume 0.1." );
            }
            return false;
    }
}

bool lexer_scan(const Source *source, TokenArray *tokens, ErrorList *errors) {
    Lexer lexer;
    if (source == NULL || tokens == NULL || errors == NULL || source->bytes == NULL ||
        tokens->count != 0U || errors->count != 0U) return false;
    lexer.source = source; lexer.tokens = tokens; lexer.errors = errors;
    lexer.start = 0U; lexer.current = 0U; lexer.start_line = 1U;
    lexer.start_column = 1U; lexer.line = 1U; lexer.column = 1U;
    if (source->length >= 3U && (unsigned char)source->bytes[0] == 0xEFU &&
        (unsigned char)source->bytes[1] == 0xBBU && (unsigned char)source->bytes[2] == 0xBFU) {
        lexer.current = 3U;
    }
    while (!at_end(&lexer)) {
        lexer.start = lexer.current; lexer.start_line = lexer.line; lexer.start_column = lexer.column;
        if (!scan_one(&lexer)) return false;
    }
    lexer.start = lexer.current; lexer.start_line = lexer.line; lexer.start_column = lexer.column;
    return add_token(&lexer, TOKEN_EOF);
}

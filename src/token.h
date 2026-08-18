#ifndef LUME_TOKEN_H
#define LUME_TOKEN_H
#include "error.h"
#include "source.h"
typedef enum {
    TOKEN_EOF, TOKEN_NEWLINE, TOKEN_IDENTIFIER, TOKEN_INTEGER, TOKEN_DECIMAL,
    TOKEN_STRING, TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE, TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_COLON, TOKEN_SEMICOLON, TOKEN_PLUS,
    TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT, TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_KW_VARIAVEL, TOKEN_KW_CONSTANTE,
    TOKEN_KW_SE, TOKEN_KW_SENAO, TOKEN_KW_ENQUANTO, TOKEN_KW_PARA, TOKEN_KW_DE,
    TOKEN_KW_ATE, TOKEN_KW_FUNCAO, TOKEN_KW_RETORNE, TOKEN_KW_VERDADEIRO,
    TOKEN_KW_FALSO, TOKEN_KW_NULO, TOKEN_KW_E, TOKEN_KW_OU, TOKEN_KW_NAO,
    TOKEN_KW_IMPORTE, TOKEN_KW_EXPORTE
} TokenType;
/* Lexeme is source->bytes + span.start.offset; Source must outlive this token. */
typedef struct { TokenType type; const Source *source; SourceSpan span; } Token;
typedef struct { Token *data; size_t count; size_t capacity; } TokenArray;
void token_array_init(TokenArray *array);
bool token_array_add(TokenArray *array, Token token);
void token_array_free(TokenArray *array);
size_t token_length(const Token *token);
const char *token_lexeme(const Token *token);
const char *token_type_name(TokenType type);
#endif

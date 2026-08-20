#include "token.h"
#include "memory.h"
void token_array_init(TokenArray *array) {
    if (array != NULL) { array->data = NULL; array->count = 0U; array->capacity = 0U; }
}
bool token_array_add(TokenArray *array, Token token) {
    Token *grown; size_t capacity;
    if (array == NULL) return false;
    if (array->count == array->capacity) {
        if (array->count == SIZE_MAX) return false;
        if (!memory_grow_capacity(array->capacity, array->count + 1U, &capacity)) return false;
        grown = memory_reallocate_array(array->data, capacity, sizeof(*grown));
        if (grown == NULL) return false;
        array->data = grown; array->capacity = capacity;
    }
    array->data[array->count++] = token; return true;
}
void token_array_free(TokenArray *array) {
    if (array != NULL) { memory_free(array->data); token_array_init(array); }
}
size_t token_length(const Token *token) {
    return token == NULL ? 0U : token->span.end.offset - token->span.start.offset;
}
const char *token_lexeme(const Token *token) {
    if (token == NULL || token->source == NULL || token->source->bytes == NULL) return NULL;
    return token->source->bytes + token->span.start.offset;
}
const char *token_type_name(TokenType type) {
    static const char *const names[] = {
        "EOF", "NEWLINE", "IDENTIFIER", "INTEGER", "DECIMAL", "STRING",
        "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE", "LEFT_BRACKET", "RIGHT_BRACKET", "COMMA", "DOT",
        "COLON", "SEMICOLON", "PLUS", "MINUS", "STAR", "SLASH", "PERCENT",
        "EQUAL", "EQUAL_EQUAL", "BANG_EQUAL", "GREATER", "GREATER_EQUAL",
        "LESS", "LESS_EQUAL", "KW_VARIAVEL", "KW_CONSTANTE", "KW_SE",
        "KW_SENAO", "KW_ENQUANTO", "KW_PARA", "KW_DE", "KW_ATE",
        "KW_FUNCAO", "KW_RETORNE", "KW_VERDADEIRO", "KW_FALSO", "KW_NULO",
        "KW_E", "KW_OU", "KW_NAO", "KW_IMPORTE", "KW_EXPORTE", "KW_EM",
        "KW_PARE", "KW_CONTINUE"
    };
    size_t index = (size_t)type;
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "UNKNOWN";
}

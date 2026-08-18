#include <stdio.h>
#include <string.h>
#include "lexer.h"

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #condition); failures++; \
} } while (0)

typedef struct { Source source; TokenArray tokens; ErrorList errors; bool ok; } Scan;
static Scan scan_text(const char *text) {
    Scan scan;
    source_init(&scan.source); token_array_init(&scan.tokens); error_list_init(&scan.errors);
    scan.ok = source_from_bytes(&scan.source, "teste.lume", text, strlen(text));
    if (scan.ok) scan.ok = lexer_scan(&scan.source, &scan.tokens, &scan.errors);
    return scan;
}
static void scan_free(Scan *scan) {
    error_list_free(&scan->errors); token_array_free(&scan->tokens); source_free(&scan->source);
}
static void expect_types(const char *text, const TokenType *expected, size_t count) {
    Scan scan = scan_text(text); size_t index;
    CHECK(scan.ok); CHECK(scan.errors.count == 0U); CHECK(scan.tokens.count == count);
    for (index = 0U; index < count && index < scan.tokens.count; index++)
        CHECK(scan.tokens.data[index].type == expected[index]);
    scan_free(&scan);
}
static void test_basics(void) {
    static const TokenType empty[] = {TOKEN_EOF};
    static const TokenType identifiers[] = {TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF};
    static const TokenType numbers[] = {TOKEN_INTEGER, TOKEN_INTEGER, TOKEN_INTEGER, TOKEN_DECIMAL, TOKEN_EOF};
    static const TokenType strings[] = {TOKEN_STRING, TOKEN_STRING, TOKEN_STRING, TOKEN_EOF};
    static const TokenType keyword[] = {TOKEN_KW_VARIAVEL, TOKEN_EOF};
    expect_types("", empty, 1U);
    expect_types("contador escreva leia", identifiers, 4U);
    expect_types("variavel", keyword, 2U);
    expect_types("0 42 999999 3.14", numbers, 5U);
    expect_types("\"Ola\" \"linha\\nnova\" \"\"", strings, 4U);
}
static void test_keywords(void) {
    static const TokenType types[] = {
        TOKEN_KW_VARIAVEL, TOKEN_KW_CONSTANTE, TOKEN_KW_SE, TOKEN_KW_SENAO,
        TOKEN_KW_ENQUANTO, TOKEN_KW_PARA, TOKEN_KW_DE, TOKEN_KW_ATE,
        TOKEN_KW_FUNCAO, TOKEN_KW_RETORNE, TOKEN_KW_VERDADEIRO, TOKEN_KW_FALSO,
        TOKEN_KW_NULO, TOKEN_KW_E, TOKEN_KW_OU, TOKEN_KW_NAO, TOKEN_EOF
    };
    expect_types("variavel constante se senao enquanto para de ate funcao retorne verdadeiro falso nulo e ou nao",
        types, sizeof(types) / sizeof(types[0]));
}
static void test_symbols(void) {
    static const TokenType types[] = {
        TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
        TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET, TOKEN_COMMA, TOKEN_COLON, TOKEN_SEMICOLON, TOKEN_PLUS, TOKEN_MINUS,
        TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT, TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
        TOKEN_BANG_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER,
        TOKEN_GREATER_EQUAL, TOKEN_EOF
    };
    expect_types("( ) { } [ ] , : ; + - * / % = == != < <= > >=", types,
        sizeof(types) / sizeof(types[0]));
}
static void test_lines_comments_and_location(void) {
    Scan scan = scan_text("variavel x = 10 // comentario UTF-8: ol\xC3\xA1\nvariavel y = 20\n");
    CHECK(scan.ok); CHECK(scan.tokens.count == 11U);
    CHECK(scan.tokens.data[0].span.start.line == 1U);
    CHECK(scan.tokens.data[0].span.start.column == 1U);
    CHECK(scan.tokens.data[1].span.start.column == 10U);
    CHECK(scan.tokens.data[4].type == TOKEN_NEWLINE);
    CHECK(scan.tokens.data[5].span.start.line == 2U);
    CHECK(scan.tokens.data[5].span.start.column == 1U);
    CHECK(scan.tokens.data[10].type == TOKEN_EOF);
    CHECK(scan.tokens.data[10].span.start.line == 3U);
    scan_free(&scan);
}
static void expect_error(const char *text) {
    Scan scan = scan_text(text);
    CHECK(!scan.ok); CHECK(scan.errors.count == 1U);
    if (scan.errors.count > 0U) CHECK(scan.errors.data[0].kind == LUME_ERROR_LEXICAL);
    scan_free(&scan);
}
static void test_errors(void) {
    expect_error("variavel pre\xC3\xA7o = 10");
    expect_error("\"nao terminada");
    expect_error("\"linha\nnova\"");
    expect_error("\"escape\\x\"");
    expect_error("123abc");
    expect_error("1.");
    expect_error(".5");
    expect_error("1..2");
    expect_error("!");
}
static void test_eof_boundaries(void) {
    static const char *const inputs[] = {"a", "7", "/", "=", "<", ">", "// fim"};
    size_t index;
    for (index = 0U; index < sizeof(inputs) / sizeof(inputs[0]); index++) {
        Scan scan = scan_text(inputs[index]);
        CHECK(scan.ok); CHECK(scan.errors.count == 0U);
        CHECK(scan.tokens.count >= 1U);
        CHECK(scan.tokens.data[scan.tokens.count - 1U].type == TOKEN_EOF);
        scan_free(&scan);
    }
}
static void test_bom_and_file(void) {
    static const TokenType bom[] = {TOKEN_KW_VARIAVEL, TOKEN_EOF};
    Source source; TokenArray tokens; ErrorList errors; bool ok;
    expect_types("\xEF\xBB\xBFvariavel", bom, 2U);
    source_init(&source); token_array_init(&tokens); error_list_init(&errors);
    ok = source_load_file(&source, "exemplos/ola.lume");
    CHECK(ok);
    if (ok) { ok = lexer_scan(&source, &tokens, &errors); CHECK(ok); CHECK(tokens.count > 1U); }
    error_list_free(&errors); token_array_free(&tokens); source_free(&source);
}
int main(void) {
    test_basics(); test_keywords(); test_symbols(); test_lines_comments_and_location();
    test_errors(); test_eof_boundaries(); test_bom_and_file();
    if (failures == 0) { puts("Todos os testes do lexer passaram."); return 0; }
    fprintf(stderr, "%d teste(s) falharam.\n", failures); return 1;
}

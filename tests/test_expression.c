#include <math.h>
#include <stdio.h>
#include <string.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #condition); failures++; \
} } while (0)

typedef struct {
    Source source; TokenArray tokens; ErrorList errors; Expr *expression; Value value; bool ok;
} Evaluation;
static Evaluation evaluate_text(const char *text) {
    Evaluation item;
    source_init(&item.source); token_array_init(&item.tokens); error_list_init(&item.errors);
    item.expression = NULL; item.value = value_null();
    item.ok = source_from_bytes(&item.source, "expressao.lume", text, strlen(text));
    if (item.ok) item.ok = lexer_scan(&item.source, &item.tokens, &item.errors);
    if (item.ok) item.ok = parser_parse_expression(&item.tokens, &item.expression, &item.errors);
    if (item.ok) item.ok = interpreter_evaluate_expression(item.expression, &item.value, &item.errors);
    return item;
}
static void evaluation_free(Evaluation *item) {
    value_free(&item->value); expr_free(item->expression); error_list_free(&item->errors);
    token_array_free(&item->tokens); source_free(&item->source);
}
static void expect_integer(const char *text, int64_t expected) {
    Evaluation item = evaluate_text(text);
    CHECK(item.ok); CHECK(item.value.type == VALUE_INTEGER);
    if (item.value.type == VALUE_INTEGER) CHECK(item.value.as.integer == expected);
    evaluation_free(&item);
}
static void expect_decimal(const char *text, double expected) {
    Evaluation item = evaluate_text(text);
    CHECK(item.ok); CHECK(item.value.type == VALUE_DECIMAL);
    if (item.value.type == VALUE_DECIMAL) CHECK(fabs(item.value.as.decimal - expected) < 1e-12);
    evaluation_free(&item);
}
static void expect_boolean(const char *text, bool expected) {
    Evaluation item = evaluate_text(text);
    CHECK(item.ok); CHECK(item.value.type == VALUE_BOOLEAN);
    if (item.value.type == VALUE_BOOLEAN) CHECK(item.value.as.boolean == expected);
    evaluation_free(&item);
}
static void expect_error(const char *text) {
    Evaluation item = evaluate_text(text);
    CHECK(!item.ok); CHECK(item.errors.count == 1U);
    evaluation_free(&item);
}
static void test_ast_shape(void) {
    Evaluation first = evaluate_text("2 + 3 * 4");
    CHECK(first.ok); CHECK(first.expression != NULL && first.expression->type == EXPR_BINARY);
    if (first.expression != NULL && first.expression->type == EXPR_BINARY) {
        CHECK(first.expression->as.binary.operator_type == BINARY_ADD);
        CHECK(first.expression->as.binary.right->type == EXPR_BINARY);
        CHECK(first.expression->as.binary.right->as.binary.operator_type == BINARY_MULTIPLY);
    }
    evaluation_free(&first);
    {
        Evaluation grouped = evaluate_text("(2 + 3) * 4");
        CHECK(grouped.ok); CHECK(grouped.expression != NULL && grouped.expression->type == EXPR_BINARY);
        if (grouped.expression != NULL && grouped.expression->type == EXPR_BINARY)
            CHECK(grouped.expression->as.binary.left->type == EXPR_GROUPING);
        evaluation_free(&grouped);
    }
    {
        Evaluation left = evaluate_text("10 - 3 - 2");
        CHECK(left.ok); CHECK(left.expression != NULL && left.expression->type == EXPR_BINARY);
        if (left.expression != NULL && left.expression->type == EXPR_BINARY)
            CHECK(left.expression->as.binary.left->type == EXPR_BINARY);
        evaluation_free(&left);
    }
    {
        Evaluation unary = evaluate_text("-5 * 2");
        CHECK(unary.ok); CHECK(unary.expression != NULL && unary.expression->type == EXPR_BINARY);
        if (unary.expression != NULL && unary.expression->type == EXPR_BINARY)
            CHECK(unary.expression->as.binary.left->type == EXPR_UNARY);
        evaluation_free(&unary);
    }
}
static void test_arithmetic(void) {
    expect_integer("2 + 2", 4); expect_integer("2 + 3 * 4", 14);
    expect_integer("(2 + 3) * 4", 20); expect_integer("10 - 3 - 2", 5);
    expect_decimal("10 / 4", 2.5); expect_integer("10 % 3", 1);
    expect_decimal("10 + 2.5", 12.5); expect_decimal("2.5 * 2", 5.0);
    expect_integer("+5", 5); expect_integer("-(2 + 3)", -5);
}
static void test_boolean_and_comparison(void) {
    expect_boolean("verdadeiro == verdadeiro", true);
    expect_boolean("falso == verdadeiro", false);
    expect_boolean("nao falso", true);
    expect_boolean("10 > 5", true); expect_boolean("10 <= 10", true);
    expect_boolean("10 == 10.0", true);
    expect_boolean("10 < 10.5", true);
    expect_boolean("10 == \"10\"", false);
    expect_boolean("verdadeiro e falso", false);
    expect_boolean("falso ou verdadeiro", true);
    expect_boolean("falso e (10 / 0 == 1)", false);
    expect_boolean("verdadeiro ou (10 / 0 == 1)", true);
}
static void test_literals(void) {
    Evaluation item = evaluate_text("nulo");
    CHECK(item.ok); CHECK(item.value.type == VALUE_NULL);
    evaluation_free(&item);
    item = evaluate_text("3.14");
    CHECK(item.ok); CHECK(item.value.type == VALUE_DECIMAL);
    evaluation_free(&item);
}
static void test_strings(void) {
    Evaluation item;
    expect_boolean("\"oi\" == \"oi\"", true);
    expect_boolean("\"oi\" != \"tchau\"", true);
    item = evaluate_text("\"Ola, \" + \"mundo\"");
    CHECK(item.ok); CHECK(item.value.type == VALUE_STRING);
    if (item.value.type == VALUE_STRING) {
        CHECK(item.value.as.string.length == 10U);
        CHECK(memcmp(item.value.as.string.bytes, "Ola, mundo", 10U) == 0);
    }
    evaluation_free(&item);
    item = evaluate_text("\"linha\\nnova\"");
    CHECK(item.ok); CHECK(item.value.type == VALUE_STRING);
    if (item.value.type == VALUE_STRING)
        CHECK(memcmp(item.value.as.string.bytes, "linha\nnova", 10U) == 0);
    evaluation_free(&item);
}
static void test_errors(void) {
    expect_error("10 / 0"); expect_error("10 % 0"); expect_error("10 + \"oi\"");
    expect_error("\"oi\" - \"x\""); expect_error("-\"texto\""); expect_error("()");
    expect_error("2 +"); expect_error("(2 + 3"); expect_error("2 3");
    expect_error("verdadeiro falso"); expect_error("1 e 2");
    expect_error("10.5 % 2");
    expect_error("9223372036854775807 + 1");
    expect_error("9223372036854775808");
    expect_error("3037000500 * 3037000500");
}
static void test_newlines(void) {
    expect_integer("\n2 + 2\n\n", 4);
    expect_integer("(2 +\n3) * 4", 20);
}
int main(void) {
    test_ast_shape(); test_arithmetic(); test_boolean_and_comparison();
    test_literals(); test_strings(); test_errors(); test_newlines();
    if (failures == 0) { puts("Todos os testes de expressoes passaram."); return 0; }
    fprintf(stderr, "%d teste(s) falharam.\n", failures); return 1;
}

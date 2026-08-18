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
    Source source; TokenArray tokens; ErrorList errors; Program *program;
    Environment environment; bool ok;
} Run;
static Run run_text(const char *text) {
    Run run;
    source_init(&run.source); token_array_init(&run.tokens); error_list_init(&run.errors);
    run.program = NULL; environment_init(&run.environment, NULL);
    run.ok = source_from_bytes(&run.source, "programa.lume", text, strlen(text));
    if (run.ok) run.ok = lexer_scan(&run.source, &run.tokens, &run.errors);
    if (run.ok) run.ok = parser_parse_program(&run.tokens, &run.program, &run.errors);
    if (run.ok) run.ok = interpreter_execute_program(run.program, &run.environment, &run.errors);
    return run;
}
static void run_free(Run *run) {
    environment_free(&run->environment); program_free(run->program);
    error_list_free(&run->errors); token_array_free(&run->tokens); source_free(&run->source);
}
static bool get_value(Run *run, const char *name, Value *out) {
    SourceSpan span = {{0U, 1U, 1U}, {0U, 1U, 1U}};
    return environment_get(&run->environment, name, strlen(name), out, span, &run->errors);
}
static void expect_integer(const char *program, const char *name, int64_t expected) {
    Run run = run_text(program);
    Value value = value_null();
    if (!run.ok) {
        fprintf(stderr, "Programa que falhou ao executar:\n%s\n", program);
        if (run.errors.count > 0U) fprintf(stderr, "Diagnostico: %s\n", run.errors.data[0].message);
    }
    CHECK(run.ok); CHECK(get_value(&run, name, &value));
    CHECK(value.type == VALUE_INTEGER);
    if (value.type == VALUE_INTEGER) CHECK(value.as.integer == expected);
    value_free(&value); run_free(&run);
}
static void expect_error(const char *program) {
    Run run = run_text(program);
    CHECK(!run.ok); CHECK(run.errors.count == 1U);
    run_free(&run);
}
static void test_variables(void) {
    expect_integer("variavel x = 10\n", "x", 10);
    expect_integer("variavel x = 10\nx = 20\n", "x", 20);
    expect_integer("variavel x = 10\nx = x + 5\n", "x", 15);
    expect_integer("variavel resultado = 2 + 3 * 4\n", "resultado", 14);
    expect_integer("variavel x = 10; variavel y = 20; x = x + y", "x", 30);
    expect_integer("variavel total =\n 10 +\n 20\n", "total", 30);
    expect_integer(
        "variavel a = 1\nvariavel b = 2\nvariavel c = 3\nvariavel d = 4\n"
        "variavel item_e = 5\nvariavel f = 6\nvariavel g = 7\nvariavel h = 8\n",
        "h", 8);
}
static void test_strings_and_constants(void) {
    Run run = run_text("variavel nome = \"Ana\"\nnome = nome + \" Maria\"\n");
    Value value = value_null();
    CHECK(run.ok); CHECK(get_value(&run, "nome", &value));
    CHECK(value.type == VALUE_STRING);
    if (value.type == VALUE_STRING) {
        CHECK(value.as.string.length == 9U);
        CHECK(memcmp(value.as.string.bytes, "Ana Maria", 9U) == 0);
    }
    value_free(&value); run_free(&run);
    run = run_text("constante PI = 3.14\n");
    CHECK(run.ok); CHECK(get_value(&run, "PI", &value)); CHECK(value.type == VALUE_DECIMAL);
    value_free(&value); run_free(&run);
    expect_error("constante PI = 3.14\nPI = 10\n");
}
static void test_scope(void) {
    Run run = run_text("variavel x = 10\n{\n variavel y = 20\n}\n");
    Value value = value_null();
    CHECK(run.ok); CHECK(!get_value(&run, "y", &value));
    value_free(&value); run_free(&run);
    expect_integer("variavel x = 10\n{\n variavel x = 20\n x = 30\n}\n", "x", 10);
    expect_integer("variavel x = 10\n{\n x = 20\n}\n", "x", 20);
    expect_error("constante x = 10\n{\n x = 20\n}\n");
    run = run_text("{ variavel escreva = 10 }\n");
    CHECK(run.ok); run_free(&run);
}
static void test_name_errors(void) {
    expect_error("x\n"); expect_error("x = 10\n");
    expect_error("variavel x = 10\nvariavel x = 20\n");
    expect_error("constante x = 10\nvariavel x = 20\n");
    expect_error("variavel x = x + 1\n");
    expect_error("variavel escreva = 10\n");
    expect_error("constante leia = 20\n");
}
static void test_parse_errors(void) {
    expect_error("variavel\n"); expect_error("variavel x\n");
    expect_error("variavel = 10\n"); expect_error("variavel x =\n");
    expect_error("constante x\n"); expect_error("x =\n");
    expect_error("{\nvariavel x = 10\n");
    expect_error("variavel x = 10 variavel y = 20\n");
}
static void test_program_ast(void) {
    Source source; TokenArray tokens; ErrorList errors; Program *program = NULL; bool ok;
    source_init(&source); token_array_init(&tokens); error_list_init(&errors);
    ok = source_from_bytes(&source, "ast.lume", "\n2 + 2\n{ 3 }\n", 13U);
    if (ok) ok = lexer_scan(&source, &tokens, &errors);
    if (ok) ok = parser_parse_program(&tokens, &program, &errors);
    CHECK(ok); CHECK(program != NULL && program->statements.count == 2U);
    if (program != NULL && program->statements.count == 2U)
        CHECK(program->statements.data[1]->type == STMT_BLOCK);
    program_free(program); error_list_free(&errors); token_array_free(&tokens); source_free(&source);
}
int main(void) {
    test_variables(); test_strings_and_constants(); test_scope();
    test_name_errors(); test_parse_errors(); test_program_ast();
    if (failures == 0) { puts("Todos os testes de programa passaram."); return 0; }
    fprintf(stderr, "%d teste(s) falharam.\n", failures); return 1;
}

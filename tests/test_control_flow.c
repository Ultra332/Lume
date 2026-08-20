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
    Source source;
    TokenArray tokens;
    ErrorList errors;
    Program *program;
    Environment environment;
    bool ok;
} Run;

static Run run_text(const char *text) {
    Run run;
    source_init(&run.source);
    token_array_init(&run.tokens);
    error_list_init(&run.errors);
    run.program = NULL;
    environment_init(&run.environment, NULL);
    run.ok = source_from_bytes(&run.source, "controle.lume", text, strlen(text));
    if (run.ok) run.ok = lexer_scan(&run.source, &run.tokens, &run.errors);
    if (run.ok) run.ok = parser_parse_program(&run.tokens, &run.program, &run.errors);
    if (run.ok) run.ok = interpreter_execute_program(run.program, &run.environment, &run.errors);
    return run;
}
static void run_free(Run *run) {
    environment_free(&run->environment);
    program_free(run->program);
    error_list_free(&run->errors);
    token_array_free(&run->tokens);
    source_free(&run->source);
}
static bool get_integer(Run *run, const char *name, int64_t *out) {
    SourceSpan span = {{0U, 1U, 1U}, {0U, 1U, 1U}, NULL};
    Value value = value_null();
    bool ok = environment_get(&run->environment, name, strlen(name), &value, span, &run->errors);
    if (ok && value.type == VALUE_INTEGER) *out = value.as.integer;
    else ok = false;
    value_free(&value);
    return ok;
}
static void expect_integer(const char *text, const char *name, int64_t expected) {
    Run run = run_text(text);
    int64_t actual = 0;
    if (!run.ok && run.errors.count > 0U)
        fprintf(stderr, "Diagnostico inesperado: %s\n", run.errors.data[0].message);
    CHECK(run.ok);
    CHECK(get_integer(&run, name, &actual));
    CHECK(actual == expected);
    run_free(&run);
}
static void expect_error(const char *text) {
    Run run = run_text(text);
    CHECK(!run.ok);
    CHECK(run.errors.count == 1U);
    run_free(&run);
}

static void test_if(void) {
    expect_integer("variavel x = 0\nse verdadeiro {\n x = 10\n}\n", "x", 10);
    expect_integer("variavel x = 0\nse falso {\n x = 10\n}\n", "x", 0);
    expect_integer(
        "variavel x = 0\nse falso {\n x = 10\n} senao {\n x = 20\n}\n",
        "x", 20);
    expect_integer(
        "variavel nota = 6\nvariavel resultado = 0\n"
        "se nota >= 7 {\n resultado = 1\n"
        "} senao se nota >= 5 {\n resultado = 2\n"
        "} senao {\n resultado = 3\n}\n",
        "resultado", 2);
    expect_integer(
        "variavel x = 0\nse falso e (10 / 0 == 1) {\n x = 1\n}\n"
        "se verdadeiro ou (10 / 0 == 1) {\n x = 2\n}\n",
        "x", 2);
    {
        Run run = run_text("se verdadeiro {\n variavel local = 10\n}\nlocal\n");
        CHECK(!run.ok);
        run_free(&run);
    }
    {
        Run run = run_text(
            "se verdadeiro {\n se falso {\n  variavel x = 1\n"
            " } senao {\n  variavel x = 10\n }\n}\n");
        CHECK(run.ok);
        run_free(&run);
    }
}
static void test_while(void) {
    expect_integer(
        "variavel i = 0\nenquanto i < 5 {\n i = i + 1\n}\n",
        "i", 5);
    {
        Run run = run_text(
            "variavel soma = 0\nvariavel i = 1\n"
            "enquanto i <= 5 {\n soma = soma + i\n i = i + 1\n}\n");
        int64_t sum = 0;
        int64_t index = 0;
        CHECK(run.ok);
        CHECK(get_integer(&run, "soma", &sum)); CHECK(sum == 15);
        CHECK(get_integer(&run, "i", &index)); CHECK(index == 6);
        run_free(&run);
    }
    {
        Run run = run_text(
            "variavel i = 0\nenquanto i < 2 {\n"
            " variavel local = i\n i = i + 1\n}\n");
        CHECK(run.ok);
        run_free(&run);
    }
    expect_integer(
        "variavel externo = 0\nvariavel i = 0\n"
        "enquanto i < 2 {\n variavel j = 0\n"
        " enquanto j < 2 {\n  externo = externo + 1\n  j = j + 1\n }\n"
        " i = i + 1\n}\n",
        "externo", 4);
}
static void test_for(void) {
    expect_integer(
        "variavel soma = 0\npara i de 1 ate 5 {\n soma = soma + i\n}\n",
        "soma", 15);
    expect_integer(
        "variavel contador = 0\npara i de 5 ate 1 {\n contador = contador + 1\n}\n",
        "contador", 0);
    expect_integer(
        "variavel contador = 0\npara i de 1 ate 1 {\n contador = contador + 1\n}\n",
        "contador", 1);
    expect_integer(
        "variavel i = 100\npara i de 1 ate 3 {\n}\nvariavel resultado = i\n",
        "resultado", 100);
    expect_integer(
        "variavel total = 0\npara i de 1 ate 3 {\n"
        " para j de 1 ate 2 {\n  variavel produto = i * j\n"
        "  total = total + produto\n }\n}\n",
        "total", 18);
    expect_integer(
        "variavel contador = 0\n"
        "para i de 9223372036854775807 ate 9223372036854775807 {\n"
        " contador = contador + 1\n}\n",
        "contador", 1);
    expect_error("para i de 1 ate 3 {\n i = 10\n}\n");
    expect_error("para i de 1 ate 3 {\n}\ni\n");
}
static void test_type_errors(void) {
    expect_error("se 10 {\n}\n");
    expect_error("enquanto \"sim\" {\n}\n");
    expect_error("para i de 1.5 ate 5 {\n}\n");
    expect_error("para i de 1 ate verdadeiro {\n}\n");
}
static void test_collection_for_and_loop_control(void) {
    expect_integer("variavel total = 0\npara item em [] { total = total + 1 }\n", "total", 0);
    expect_integer("variavel total = 0\npara item em [9] { total = total + item }\n", "total", 9);
    expect_integer(
        "variavel soma = 0\npara numero em [1, 2, 3, 4] {\n soma = soma + numero\n}\n",
        "soma", 10);
    expect_integer(
        "variavel soma = 0\npara numero em [1, 2, 3, 4] {\n"
        " se numero == 2 { continue }\n se numero == 4 { pare }\n soma = soma + numero\n}\n",
        "soma", 4);
    expect_integer(
        "variavel i = 0\nvariavel soma = 0\nenquanto verdadeiro {\n i = i + 1\n"
        " se i == 2 { continue }\n se i == 4 { pare }\n soma = soma + i\n}\n",
        "soma", 4);
    expect_integer(
        "variavel itens = [1, 2, 3]\nvariavel vezes = 0\npara item em itens {\n"
        " adicione(itens, 9)\n vezes = vezes + 1\n}\n",
        "vezes", 3);
    expect_integer(
        "variavel total = 0\npara linha em [[1, 2], [3]] {\n"
        " para item em linha { total = total + item }\n}\n",
        "total", 6);
    expect_integer(
        "variavel total = 0\npara i de 1 ate 3 {\n para j de 1 ate 5 {\n"
        "  se j == 3 { pare }\n  total = total + 1\n }\n}\n",
        "total", 6);
    expect_integer(
        "funcao numeros() { retorne [2, 4, 6] }\nvariavel total = 0\n"
        "para item em numeros() { total = total + item }\n",
        "total", 12);
    expect_integer(
        "funcao fabrica() { variavel dados = [5, 7]; funcao obter() { retorne dados }; retorne obter }\n"
        "variavel obter = fabrica()\nvariavel total = 0\npara item em obter() { total = total + item }\n",
        "total", 12);
    expect_integer(
        "variavel item = 40\npara item em [1, 2] { variavel copia = item }\n"
        "variavel resultado = item\n",
        "resultado", 40);
    expect_integer(
        "variavel total = 0\npara rodada de 1 ate 10000 {\n"
        " para item em [1, 2, 3] { total = total + item }\n}\n",
        "total", 60000);
    expect_error("para item em 10 { }\n");
    expect_error("pare\n");
    expect_error("continue\n");
    expect_error("para item em [1] { funcao f() { pare } }\n");
    expect_error("para item em { }\n");
}
static void test_syntax_errors(void) {
    expect_error("se {\n}\n");
    expect_error("se verdadeiro\n");
    expect_error("se verdadeiro {\n");
    expect_error("senao {\n}\n");
    expect_error("enquanto {\n}\n");
    expect_error("enquanto verdadeiro\n");
    expect_error("para i 1 ate 5 {\n}\n");
    expect_error("para i de 1 5 {\n}\n");
    expect_error("para de 1 ate 5 {\n}\n");
    expect_error("para i de ate 5 {\n}\n");
}
static void test_ast_types(void) {
    Run run = run_text(
        "se verdadeiro {\n}\n"
        "enquanto falso {\n}\n"
        "para i de 2 ate 1 {\n}\n");
    CHECK(run.ok);
    CHECK(run.program != NULL && run.program->statements.count == 3U);
    if (run.program != NULL && run.program->statements.count == 3U) {
        CHECK(run.program->statements.data[0]->type == STMT_IF);
        CHECK(run.program->statements.data[1]->type == STMT_WHILE);
        CHECK(run.program->statements.data[2]->type == STMT_FOR);
    }
    run_free(&run);
}
int main(void) {
    test_if();
    test_while();
    test_for();
    test_collection_for_and_loop_control();
    test_type_errors();
    test_syntax_errors();
    test_ast_types();
    if (failures == 0) {
        puts("Todos os testes de controle de fluxo passaram.");
        return 0;
    }
    fprintf(stderr, "%d teste(s) falharam.\n", failures);
    return 1;
}

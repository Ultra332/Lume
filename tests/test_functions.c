#include <stdio.h>
#include <string.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)
typedef struct { Source source; TokenArray tokens; ErrorList errors; Program *program; Environment environment; FILE *output; bool ok; } Run;
static Run run_text(const char *text) {
    Run run; RuntimeIO io;
    source_init(&run.source); token_array_init(&run.tokens); error_list_init(&run.errors);
    run.program = NULL; environment_init(&run.environment, NULL); run.output = tmpfile();
    run.ok = run.output != NULL && source_from_bytes(&run.source, "funcoes.lume", text, strlen(text));
    if (run.ok) run.ok = lexer_scan(&run.source, &run.tokens, &run.errors);
    if (run.ok) run.ok = parser_parse_program(&run.tokens, &run.program, &run.errors);
    io.input = stdin; io.output = run.output;
    if (run.ok) run.ok = interpreter_execute_program_with_io(run.program, &run.environment, &io, &run.errors);
    return run;
}
static void run_free(Run *run) {
    if (run->output != NULL) fclose(run->output);
    environment_free(&run->environment); program_free(run->program); error_list_free(&run->errors);
    token_array_free(&run->tokens); source_free(&run->source);
}
static bool integer(Run *run, const char *name, int64_t expected) {
    SourceSpan span = {{0U,1U,1U},{0U,1U,1U}}; Value value = value_null();
    bool ok = environment_get(&run->environment, name, strlen(name), &value, span, &run->errors);
    ok = ok && value.type == VALUE_INTEGER && value.as.integer == expected; value_free(&value); return ok;
}
static void expect_integer(const char *text, const char *name, int64_t expected) {
    Run run = run_text(text); if (!run.ok && run.errors.count) fprintf(stderr, "%s\nPrograma:\n%s\n", run.errors.data[0].message, text);
    CHECK(run.ok); CHECK(integer(&run, name, expected)); run_free(&run);
}
static void expect_error(const char *text) { Run run = run_text(text); CHECK(!run.ok); CHECK(run.errors.count == 1U); run_free(&run); }
static void test_basics(void) {
    expect_integer("funcao soma(a,b){ retorne a+b }\nvariavel r=soma(2,3)\n", "r", 5);
    expect_integer("funcao nada(){ retorne }\nvariavel r=0\nse nada()==nulo { r=1 }\n", "r", 1);
}
static void test_recursion_and_hoisting(void) {
    expect_integer("funcao fat(n){ se n<=1 { retorne 1 }; retorne n*fat(n-1) }\nvariavel r=fat(6)\n", "r", 720);
    expect_integer("variavel r=par(10)\nfuncao par(n){ se n==0 { retorne verdadeiro }; retorne impar(n-1) }\nfuncao impar(n){ se n==0 { retorne falso }; retorne par(n-1) }\nse r { variavel x=0 }\nvariavel numero=1\n", "numero", 1);
}
static void test_closures(void) {
    expect_integer(
        "funcao fabrica(base){ funcao somar(x){ retorne base+x }; retorne somar }\n"
        "variavel f=fabrica(10)\nvariavel r=f(7)\n", "r", 17);
    expect_integer(
        "funcao contador(){ variavel n=0; funcao proximo(){ n=n+1; retorne n }; retorne proximo }\n"
        "variavel c=contador()\nvariavel a=c()\nvariavel r=c()\n", "r", 2);
}
static void test_values_calls_and_return_flow(void) {
    expect_integer("funcao id(f){ retorne f }\nfuncao um(){ retorne 1 }\nvariavel r=id(um)()\n", "r", 1);
    expect_integer("funcao busca(){ para i de 1 ate 5 { se i==3 { retorne i } }; retorne 0 }\nvariavel r=busca()\n", "r", 3);
    expect_integer("funcao fim(){ variavel x=1 }\nvariavel r=fim()==nulo\nse r { variavel local=0 }\nvariavel numero=9\n", "numero", 9);
}
static void test_native_output(void) {
    Run run = run_text("escreva(42)\nescreva(\"oi\")\n"); char buffer[32] = {0}; size_t length;
    CHECK(run.ok); rewind(run.output); length = fread(buffer, 1U, sizeof(buffer)-1U, run.output);
    CHECK(length == 6U); CHECK(memcmp(buffer, "42\noi\n", 6U) == 0); run_free(&run);
}
static void test_errors(void) {
    expect_error("retorne 1\n");
    expect_error("funcao f(a,a){ retorne a }\n");
    expect_error("funcao f(a){ retorne a }\nf()\n");
    expect_error("funcao f(){ retorne 1 }\nf(1)\n");
    expect_error("variavel x=1\nx()\n");
    expect_error("funcao f(){ retorne inexistente }\nf()\n");
    expect_error("funcao escreva(){ }\n");
}
int main(void) {
    test_basics(); test_recursion_and_hoisting(); test_closures(); test_values_calls_and_return_flow();
    test_native_output(); test_errors();
    if (failures == 0) { puts("Todos os testes de funcoes passaram."); return 0; }
    fprintf(stderr, "%d teste(s) falharam.\n", failures); return 1;
}

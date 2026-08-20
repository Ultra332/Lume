#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "session.h"

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #condition); failures++; \
} } while (0)

static bool execute(const char *code, const char *input, char *output, size_t capacity) {
    LumeSession session;
    ErrorList errors;
    Source *error_source = NULL;
    RuntimeIO io;
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    bool ok;
    size_t count;
    if (in == NULL || out == NULL) exit(2);
    if (input != NULL) { fputs(input, in); rewind(in); }
    io.input = in; io.output = out;
    session_init(&session, io); error_list_init(&errors);
    ok = session_execute(&session, "tests/v020.lume", code, strlen(code), false,
        &error_source, &errors);
    if (!ok && error_source != NULL) { source_free(error_source); memory_free(error_source); }
    error_list_free(&errors); session_free(&session);
    rewind(out); count = fread(output, 1U, capacity - 1U, out); output[count] = '\0';
    fclose(in); fclose(out);
    return ok;
}

static void test_read_prompt(void) {
    char output[512];
    CHECK(execute("variavel nome = leia(\"Nome: \")\nescreva(nome)\n", "Lume\n",
        output, sizeof(output)));
    CHECK(strcmp(output, "Nome: Lume\n") == 0);
    CHECK(execute("escreva(leia())\n", "sem convite\n", output, sizeof(output)));
    CHECK(strcmp(output, "sem convite\n") == 0);
    CHECK(!execute("leia(10)\n", "", output, sizeof(output)));
    CHECK(!execute("leia(\"a\", \"b\")\n", "", output, sizeof(output)));
}

static void test_random_module(void) {
    char output[8192];
    const char *code =
        "importe \"lume/aleatorio\"\n"
        "para i de 1 ate 100 {\n"
        " variavel n = aleatorio.inteiro(3, 7)\n"
        " se n < 3 ou n > 7 { escreva(\"fora\") }\n"
        " variavel d = aleatorio.decimal()\n"
        " se d < 0.0 ou d >= 1.0 { escreva(\"decimal fora\") }\n"
        "}\n"
        "escreva(aleatorio.escolha([\"ok\"]))\n";
    CHECK(execute(code, NULL, output, sizeof(output)));
    CHECK(strcmp(output, "ok\n") == 0);
    CHECK(!execute("importe \"lume/aleatorio\"\naleatorio.inteiro(5, 1)\n", NULL,
        output, sizeof(output)));
    CHECK(!execute("importe \"lume/aleatorio\"\naleatorio.escolha([])\n", NULL,
        output, sizeof(output)));
}

static void test_terminal_module(void) {
    char output[2048];
    const char *code =
        "importe \"lume/terminal\"\n"
        "terminal.limpe()\nterminal.posicione(2, 3)\n"
        "terminal.oculte_cursor()\nterminal.mostre_cursor()\n"
        "escreva(tamanho(terminal.tamanho()))\n"
        "escreva(terminal.leia_tecla())\n";
    CHECK(execute(code, "x", output, sizeof(output)));
    CHECK(strstr(output, "\x1b[2J\x1b[H") != NULL);
    CHECK(strstr(output, "\x1b[3;2H") != NULL);
    CHECK(strstr(output, "\x1b[?25l\x1b[?25h") != NULL);
    CHECK(strstr(output, "2\nx\n") != NULL);
    CHECK(!execute("importe \"lume/terminal\"\nterminal.posicione(0, 1)\n", NULL,
        output, sizeof(output)));
    CHECK(!execute("importe \"lume/terminal\"\nterminal.oculte_cursor()\nterminal.posicione(0, 1)\n", NULL,
        output, sizeof(output)));
    CHECK(strstr(output, "\x1b[?25l\x1b[?25h") != NULL);
}

int main(void) {
    test_read_prompt(); test_random_module(); test_terminal_module();
    if (failures == 0) { puts("Todos os testes da Lume v0.2.0 passaram."); return 0; }
    fprintf(stderr, "%d teste(s) falharam.\n", failures); return 1;
}

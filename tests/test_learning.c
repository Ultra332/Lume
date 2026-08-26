#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "learn.h"

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static size_t read_output(FILE *file, char *buffer, size_t capacity) {
    size_t count;
    rewind(file);
    count = fread(buffer, 1U, capacity - 1U, file);
    buffer[count] = '\0';
    return count;
}

static int run_learning(int argc, char **argv, const char *input,
                        char *buffer, size_t capacity) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    RuntimeIO io = {in, out};
    int result;
    if (input != NULL) fputs(input, in);
    rewind(in);
    result = learn_cli_from_directory(argc, argv, io, "conteudo/aprender");
    (void)read_output(out, buffer, capacity);
    fclose(in);
    fclose(out);
    return result;
}

static void test_list_and_interactive_menu(void) {
    char buffer[16384];
    char *list[] = {"listar"};
    CHECK(run_learning(1, list, NULL, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Aprender Lume") != NULL);
    CHECK(strstr(buffer, "01. Seu primeiro programa") != NULL);
    CHECK(strstr(buffer, "07. Listas") != NULL);
    CHECK(run_learning(0, NULL, "1\n", buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Licao 01") != NULL);
    CHECK(strstr(buffer, "Olá, mundo!") != NULL);
}

static void test_open_next_challenge_and_hint(void) {
    char buffer[16384];
    char *open[] = {"abrir", "02"};
    char *next[] = {"proxima", "02"};
    char *challenge[] = {"desafio", "04"};
    char *hint[] = {"dica", "04"};
    CHECK(run_learning(2, open, NULL, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "## Objetivo") != NULL);
    CHECK(strstr(buffer, "variavel nome") != NULL);
    CHECK(run_learning(2, next, NULL, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Licao 03") != NULL);
    CHECK(strstr(buffer, "Entrada de dados") != NULL);
    CHECK(run_learning(2, challenge, NULL, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "## Desafio") != NULL);
    CHECK(strstr(buffer, "## Dica") == NULL);
    CHECK(run_learning(2, hint, NULL, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "## Dica") != NULL);
    CHECK(strstr(buffer, "maior ou igual") != NULL);
}

static void test_missing_and_corrupt_content(void) {
    char buffer[4096];
    char *missing_lesson[] = {"99"};
    char *open[] = {"01"};
    FILE *out = tmpfile();
    RuntimeIO io = {stdin, out};
    CHECK(run_learning(1, missing_lesson, NULL, buffer, sizeof(buffer)) == 1);
    CHECK(strstr(buffer, "nao existe") != NULL);
    CHECK(learn_cli_from_directory(1, open, io, "tests/fixtures/missing_learning") == 1);
    (void)read_output(out, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "ausente") != NULL);
    fclose(out);
    out = tmpfile();
    io.output = out;
    CHECK(learn_cli_from_directory(1, open, io, "tests/fixtures/learning_corrupt") == 1);
    (void)read_output(out, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "corrompido") != NULL);
    fclose(out);
}

static void test_cli_and_existing_education(void) {
    char buffer[16384];
    FILE *out = tmpfile();
    RuntimeIO io = {stdin, out};
    char *learning[] = {"lume", "aprender", "listar"};
    char *explain[] = {"lume", "--explicar", "exemplos/iniciante/variaveis.lume"};
    char *analyze[] = {"lume", "--analisar", "exemplos/iniciante/maior-de-idade.lume"};
    CHECK(cli_run(3, learning, io) == 0);
    (void)read_output(out, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "07. Listas") != NULL);
    freopen(NULL, "w+", out);
    CHECK(cli_run(3, explain, io) == 0);
    (void)read_output(out, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "Criando a variavel") != NULL);
    freopen(NULL, "w+", out);
    CHECK(cli_run(3, analyze, io) == 0);
    (void)read_output(out, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "Analisando:") != NULL);
    fclose(out);
}

static void test_documented_examples(void) {
    const char *paths[] = {
        "exemplos/iniciante/ola-mundo.lume",
        "exemplos/iniciante/soma.lume",
        "exemplos/iniciante/maior-de-idade.lume",
        "exemplos/iniciante/tabuada.lume",
        "exemplos/intermediario/funcoes.lume",
        "exemplos/intermediario/listas.lume"
    };
    size_t index;
    for (index = 0U; index < sizeof(paths) / sizeof(paths[0]); index++) {
        FILE *out = tmpfile();
        RuntimeIO io = {stdin, out};
        char *args[] = {"lume", (char *)paths[index]};
        CHECK(cli_run(2, args, io) == 0);
        fclose(out);
    }
}

int main(void) {
    test_list_and_interactive_menu();
    test_open_next_challenge_and_hint();
    test_missing_and_corrupt_content();
    test_cli_and_existing_education();
    test_documented_examples();
    if (failures == 0) {
        puts("Todos os testes do modo aprender passaram.");
        return 0;
    }
    return 1;
}

#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "learning_catalog.h"

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static void output(FILE *file, char *buffer, size_t capacity) {
    size_t count;
    rewind(file);
    count = fread(buffer, 1U, capacity - 1U, file);
    buffer[count] = '\0';
}

static int run_cli(int argc, char **argv, char *buffer, size_t capacity) {
    FILE *out = tmpfile();
    RuntimeIO io = {stdin, out};
    int result = cli_run(argc, argv, io);
    output(out, buffer, capacity);
    fclose(out);
    return result;
}

static void test_concepts(void) {
    char buffer[32768];
    char *list[] = {"lume", "aprender", "conceitos"};
    char *variable[] = {"lume", "aprender", "conceito", "variavel"};
    char *function[] = {"lume", "aprender", "conceito", "funcao"};
    char *parameter[] = {"lume", "aprender", "conceito", "parametro"};
    char *argument[] = {"lume", "aprender", "conceito", "argumento"};
    char *scope[] = {"lume", "aprender", "conceito", "escopo"};
    char *missing[] = {"lume", "aprender", "conceito", "inexistente"};
    char *accented[] = {"lume", "aprender", "conceito", "função"};
    CHECK(run_cli(3, list, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Conceitos de programação") != NULL);
    CHECK(strstr(buffer, "Variável") != NULL && strstr(buffer, "Módulo") != NULL);
    CHECK(run_cli(4, variable, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "guarda um valor") != NULL);
    CHECK(strstr(buffer, "atribuição, escopo") != NULL);
    CHECK(run_cli(4, function, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "parâmetro, argumento, retorno, escopo") != NULL);
    CHECK(run_cli(4, parameter, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "disponível dentro da função") != NULL);
    CHECK(run_cli(4, argument, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "valor entregue") != NULL);
    CHECK(run_cli(4, scope, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "região do programa") != NULL);
    CHECK(run_cli(4, accented, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Função") != NULL);
    CHECK(run_cli(4, missing, buffer, sizeof(buffer)) == 1);
    CHECK(strstr(buffer, "não foi encontrado") != NULL);
    CHECK(learning_concept_count() >= 19U);
}

static void test_transition(void) {
    char buffer[65536];
    char *list[] = {"lume", "aprender", "transicao"};
    char *python[] = {"lume", "aprender", "transicao", "python"};
    char *missing[] = {"lume", "aprender", "transicao", "lua"};
    CHECK(run_cli(3, list, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "1. Python") != NULL);
    CHECK(strstr(buffer, "Lua") == NULL);
    CHECK(run_cli(4, python, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Lume → Python") != NULL);
    CHECK(strstr(buffer, "if idade >= 18:") != NULL);
    CHECK(strstr(buffer, "def dobro(numero):") != NULL);
    CHECK(strstr(buffer, "não são equivalentes internamente") != NULL);
    CHECK(run_cli(4, missing, buffer, sizeof(buffer)) == 1);
    CHECK(strstr(buffer, "não está disponível") != NULL);
    CHECK(transition_language_count() == 1U);
}

static void test_comparison(void) {
    char buffer[65536];
    char *compare[] = {"lume", "comparar", "exemplos/v050/ponte-python.lume", "--com", "python"};
    char *module[] = {"lume", "comparar", "exemplos/modulos/principal.lume", "--com", "python"};
    char *language[] = {"lume", "comparar", "exemplos/v050/ponte-python.lume", "--com", "lua"};
    char *invalid[] = {"lume", "comparar", "tests/.tmp-v050-invalid.lume", "--com", "python"};
    char *complete[] = {"lume", "comparar", "tests/.tmp-v050-complete.lume", "--com", "python"};
    FILE *file;
    CHECK(run_cli(5, compare, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "idade = 18") != NULL);
    CHECK(strstr(buffer, "if idade >= 18:") != NULL);
    CHECK(strstr(buffer, "for nome in nomes:") != NULL);
    CHECK(strstr(buffer, "def dobro(numero):") != NULL);
    CHECK(strstr(buffer, "return numero * 2") != NULL);
    CHECK(strstr(buffer, "print(dobro(5))") != NULL);
    CHECK(strstr(buffer, "Conceitos encontrados") != NULL);
    CHECK(strstr(buffer, "não um transpiler") != NULL);
    file = fopen(complete[2], "wb");
    CHECK(file != NULL);
    if (file != NULL) {
        fputs("variavel nome = leia(\"Nome: \")\n"
              "variavel itens = [1, 2]\n"
              "itens[0] = 3\n"
              "nome = \"Ana\"\n"
              "enquanto falso {\n  escreva(nome)\n}\n"
              "para indice de 1 ate 2 {\n  escreva(itens[indice - 1])\n}\n", file);
        fclose(file);
        CHECK(run_cli(5, complete, buffer, sizeof(buffer)) == 0);
        CHECK(strstr(buffer, "input(\"Nome: \")") != NULL);
        CHECK(strstr(buffer, "itens[0] = 3") != NULL);
        CHECK(strstr(buffer, "nome = \"Ana\"") != NULL);
        CHECK(strstr(buffer, "while False:") != NULL);
        CHECK(strstr(buffer, "for indice in range(1, (2) + 1):") != NULL);
        CHECK(remove(complete[2]) == 0);
    }
    CHECK(run_cli(5, module, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "ainda não possui comparação") != NULL);
    CHECK(run_cli(5, language, buffer, sizeof(buffer)) == 1);
    file = fopen(invalid[2], "wb");
    CHECK(file != NULL);
    if (file != NULL) {
        fputs("se verdadeiro {\n", file);
        fclose(file);
        CHECK(run_cli(5, invalid, buffer, sizeof(buffer)) == 1);
        CHECK(strstr(buffer, "Erro de sintaxe") != NULL);
        CHECK(remove(invalid[2]) == 0);
    }
}

static void test_regressions(void) {
    char buffer[32768];
    char *lesson[] = {"lume", "aprender", "02"};
    char *explain[] = {"lume", "--explicar", "exemplos/iniciante/variaveis.lume"};
    char *analyze[] = {"lume", "--analisar", "exemplos/iniciante/maior-de-idade.lume"};
    CHECK(run_cli(3, lesson, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Licao 02") != NULL);
    CHECK(run_cli(3, explain, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Criando a variavel") != NULL);
    CHECK(run_cli(3, analyze, buffer, sizeof(buffer)) == 0);
    CHECK(strstr(buffer, "Analisando:") != NULL);
}

int main(void) {
    test_concepts();
    test_transition();
    test_comparison();
    test_regressions();
    if (failures == 0) {
        puts("Todos os testes educacionais da Lume v0.5.0 passaram.");
        return 0;
    }
    return 1;
}

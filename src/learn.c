#include "learn.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "memory.h"
#include "learning_catalog.h"
#include "source.h"

typedef struct {
    const char *id;
    const char *title;
    const char *file_name;
} LessonInfo;

static const LessonInfo LESSONS[] = {
    {"01", "Seu primeiro programa", "01-primeiro-programa.txt"},
    {"02", "Variáveis", "02-variaveis.txt"},
    {"03", "Entrada de dados", "03-entrada-de-dados.txt"},
    {"04", "Decisões com se", "04-decisoes.txt"},
    {"05", "Repetições", "05-repeticoes.txt"},
    {"06", "Funções", "06-funcoes.txt"},
    {"07", "Listas", "07-listas.txt"}
};

static const size_t LESSON_COUNT = sizeof(LESSONS) / sizeof(LESSONS[0]);

static const LessonInfo *find_lesson(const char *id) {
    char normalized[3];
    size_t index;
    if (id == NULL) return NULL;
    if (strlen(id) == 1U && isdigit((unsigned char)id[0])) {
        normalized[0] = '0';
        normalized[1] = id[0];
        normalized[2] = '\0';
        id = normalized;
    }
    for (index = 0U; index < LESSON_COUNT; index++) {
        if (strcmp(LESSONS[index].id, id) == 0) return &LESSONS[index];
    }
    return NULL;
}

static char *join_path(const char *directory, const char *name) {
    size_t left;
    size_t right;
    bool separator;
    char *path;
    if (directory == NULL || name == NULL) return NULL;
    left = strlen(directory);
    right = strlen(name);
    separator = left > 0U && directory[left - 1U] != '/' && directory[left - 1U] != '\\';
    if (left > SIZE_MAX - right - (separator ? 2U : 1U)) return NULL;
    path = memory_allocate(left + right + (separator ? 2U : 1U));
    if (path == NULL) return NULL;
    (void)snprintf(path, left + right + (separator ? 2U : 1U), "%s%s%s",
                   directory, separator ? "/" : "", name);
    return path;
}

static bool directory_available(const char *directory) {
    char *path = join_path(directory, LESSONS[0].file_name);
    FILE *file;
    if (path == NULL) return false;
    file = fopen(path, "rb");
    memory_free(path);
    if (file == NULL) return false;
    (void)fclose(file);
    return true;
}

static char *executable_content_directory(const char *executable_path) {
    char path[4096];
    const char *source = executable_path;
    char *slash;
    char *directory;
    size_t length;
#ifdef _WIN32
    DWORD count = GetModuleFileNameA(NULL, path, (DWORD)sizeof(path));
    if (count > 0U && count < (DWORD)sizeof(path)) {
        path[count] = '\0';
        source = path;
    }
#endif
    if (source == NULL) return NULL;
    length = strlen(source);
    if (length >= sizeof(path)) return NULL;
    if (source != path) memcpy(path, source, length + 1U);
    slash = strrchr(path, '/');
    {
        char *backslash = strrchr(path, '\\');
        if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
    }
    if (slash == NULL) return NULL;
    *slash = '\0';
    directory = join_path(path, "conteudo/aprender");
    return directory;
}

static bool lesson_is_valid(const Source *source, const LessonInfo *lesson) {
    char expected_id[16];
    char expected_title[160];
    if (source == NULL || source->bytes == NULL || lesson == NULL) return false;
    (void)snprintf(expected_id, sizeof(expected_id), "id: %s", lesson->id);
    (void)snprintf(expected_title, sizeof(expected_title), "titulo: %s", lesson->title);
    return strstr(source->bytes, expected_id) == source->bytes &&
           strstr(source->bytes, expected_title) != NULL &&
           strstr(source->bytes, "## Objetivo") != NULL &&
           strstr(source->bytes, "## Explicação") != NULL &&
           strstr(source->bytes, "## Exemplo") != NULL &&
           strstr(source->bytes, "## Desafio") != NULL &&
           strstr(source->bytes, "## Dica") != NULL &&
           strstr(source->bytes, "## Veja a execução") != NULL;
}

static bool load_lesson(const char *directory, const LessonInfo *lesson,
                        Source *source, FILE *output) {
    char *path = join_path(directory, lesson->file_name);
    bool loaded;
    if (path == NULL) {
        fputs("Nao foi possivel preparar o caminho da licao.\n", output);
        return false;
    }
    loaded = source_load_file(source, path);
    if (!loaded) {
        fprintf(output, "Conteudo da licao %s ausente: %s\n", lesson->id, path);
        memory_free(path);
        return false;
    }
    if (!lesson_is_valid(source, lesson)) {
        fprintf(output, "Conteudo da licao %s esta corrompido ou incompleto.\n", lesson->id);
        memory_free(path);
        return false;
    }
    memory_free(path);
    return true;
}

static void list_lessons(FILE *output) {
    size_t index;
    fputs("Aprender Lume\n\n", output);
    for (index = 0U; index < LESSON_COUNT; index++) {
        fprintf(output, "%s. %s\n", LESSONS[index].id, LESSONS[index].title);
    }
}

static void list_concepts(FILE *output) {
    size_t index;
    fputs("Conceitos de programação\n\n", output);
    for (index = 0U; index < learning_concept_count(); index++) {
        const LearningConcept *concept = learning_concept_at(index);
        fprintf(output, "%2zu. %s\n", index + 1U, concept->title);
    }
    fputs("\nConsulte um conceito com:\n  lume aprender conceito variavel\n", output);
}

static int show_concept(const char *slug, FILE *output) {
    const LearningConcept *concept = learning_concept_find(slug);
    if (concept == NULL) {
        fprintf(output, "O conceito '%s' não foi encontrado. Use 'lume aprender conceitos'.\n",
                slug == NULL ? "" : slug);
        return 1;
    }
    fprintf(output,
        "%s\n\n%s\n\nExemplo em Lume:\n\n%s\n\nNeste exemplo:\n%s\n\nOnde costuma ser utilizado:\n%s\n\nConceitos relacionados:\n%s\n\nLição relacionada:\n%s\n",
        concept->title, concept->definition, concept->example, concept->explanation,
        concept->uses, concept->related, concept->lesson);
    return 0;
}

static void list_transition_languages(FILE *output) {
    size_t index;
    fputs("Transição para outra linguagem\n\nLinguagens disponíveis:\n", output);
    for (index = 0U; index < transition_language_count(); index++) {
        const TransitionLanguage *language = transition_language_at(index);
        fprintf(output, "%zu. %s\n", index + 1U, language->name);
    }
    fputs("\nAbra a ponte inicial com:\n  lume aprender transicao python\n", output);
}

static int show_transition(const char *slug, FILE *output) {
    const TransitionLanguage *language = transition_language_find(slug);
    size_t index;
    if (language == NULL) {
        fprintf(output, "A linguagem '%s' não está disponível nesta versão.\n",
                slug == NULL ? "" : slug);
        return 1;
    }
    fprintf(output, "Transição: Lume → %s\n\n%s\n", language->name,
            language->introduction);
    for (index = 0U; index < language->topic_count; index++) {
        const TransitionTopic *topic = &language->topics[index];
        fprintf(output,
            "\n%d. %s\n\nLume:\n%s\n\n%s:\n%s\n\nO que mudou:\n%s\n\nO que continuou igual:\n%s\n",
            (int)(index + 1U), topic->title, topic->lume, language->name,
            topic->destination, topic->changed, topic->preserved);
    }
    fputs("\nPratique: escolha um exemplo pequeno em Lume, identifique seus conceitos e reescreva-o em Python antes de comparar a resposta.\n", output);
    return 0;
}

static int show_lesson(const char *directory, const LessonInfo *lesson, FILE *output) {
    Source source;
    const char *content;
    source_init(&source);
    if (!load_lesson(directory, lesson, &source, output)) {
        source_free(&source);
        return 1;
    }
    fprintf(output, "\nLicao %s — %s\n\n", lesson->id, lesson->title);
    content = strstr(source.bytes, "## Objetivo");
    (void)fwrite(content, 1U, source.length - (size_t)(content - source.bytes), output);
    if (source.bytes[source.length - 1U] != '\n') fputc('\n', output);
    source_free(&source);
    return 0;
}

static int show_section(const char *directory, const LessonInfo *lesson,
                        const char *heading, FILE *output) {
    Source source;
    const char *begin;
    const char *end;
    source_init(&source);
    if (!load_lesson(directory, lesson, &source, output)) {
        source_free(&source);
        return 1;
    }
    begin = strstr(source.bytes, heading);
    if (begin == NULL) {
        source_free(&source);
        return 1;
    }
    end = strstr(begin + strlen(heading), "\n## ");
    if (end == NULL) end = source.bytes + source.length;
    (void)fwrite(begin, 1U, (size_t)(end - begin), output);
    fputc('\n', output);
    source_free(&source);
    return 0;
}

static int missing_lesson(const char *id, FILE *output) {
    fprintf(output, "A licao '%s' nao existe. Use 'lume aprender listar'.\n",
            id == NULL ? "" : id);
    return 1;
}

int learn_cli_from_directory(int argc, char **argv, RuntimeIO io,
                             const char *content_directory) {
    const LessonInfo *lesson;
    char choice[32];
    if (argc == 1 && (strcmp(argv[0], "conceitos") == 0 || strcmp(argv[0], "conceito") == 0)) {
        list_concepts(io.output);
        return 0;
    }
    if (argc == 2 && strcmp(argv[0], "conceito") == 0)
        return show_concept(argv[1], io.output);
    if (argc == 1 && strcmp(argv[0], "transicao") == 0) {
        list_transition_languages(io.output);
        return 0;
    }
    if (argc == 2 && strcmp(argv[0], "transicao") == 0)
        return show_transition(argv[1], io.output);
    if (content_directory == NULL) {
        fputs("Os conteudos de aprendizado nao foram encontrados. Reinstale a Lume ou mantenha a pasta conteudo ao lado do executavel.\n", io.output);
        return 1;
    }
    if (argc == 0) {
        list_lessons(io.output);
        fputs("\nEscolha uma licao: ", io.output);
        if (fgets(choice, sizeof(choice), io.input) == NULL) {
            fputs("\nNenhuma licao selecionada.\n", io.output);
            return 0;
        }
        choice[strcspn(choice, "\r\n")] = '\0';
        lesson = find_lesson(choice);
        return lesson == NULL ? missing_lesson(choice, io.output)
                              : show_lesson(content_directory, lesson, io.output);
    }
    if (argc == 1 && (strcmp(argv[0], "listar") == 0 || strcmp(argv[0], "lista") == 0)) {
        list_lessons(io.output);
        return 0;
    }
    if (argc == 1) {
        lesson = find_lesson(argv[0]);
        return lesson == NULL ? missing_lesson(argv[0], io.output)
                              : show_lesson(content_directory, lesson, io.output);
    }
    if (argc == 2 && (strcmp(argv[0], "abrir") == 0 || strcmp(argv[0], "licao") == 0)) {
        lesson = find_lesson(argv[1]);
        return lesson == NULL ? missing_lesson(argv[1], io.output)
                              : show_lesson(content_directory, lesson, io.output);
    }
    if (argc == 2 && strcmp(argv[0], "proxima") == 0) {
        lesson = find_lesson(argv[1]);
        if (lesson == NULL) return missing_lesson(argv[1], io.output);
        if ((size_t)(lesson - LESSONS) + 1U >= LESSON_COUNT) {
            fputs("Voce concluiu a trilha inicial da Lume.\n", io.output);
            return 0;
        }
        return show_lesson(content_directory, lesson + 1, io.output);
    }
    if (argc == 2 && strcmp(argv[0], "desafio") == 0) {
        lesson = find_lesson(argv[1]);
        return lesson == NULL ? missing_lesson(argv[1], io.output)
                              : show_section(content_directory, lesson, "## Desafio", io.output);
    }
    if (argc == 2 && strcmp(argv[0], "dica") == 0) {
        lesson = find_lesson(argv[1]);
        return lesson == NULL ? missing_lesson(argv[1], io.output)
                              : show_section(content_directory, lesson, "## Dica", io.output);
    }
    fputs("Uso: lume aprender [listar | conceito [nome] | conceitos | transicao [linguagem] | ID | abrir ID | proxima ID | desafio ID | dica ID]\n", io.output);
    return 2;
}

int learn_cli(int argc, char **argv, RuntimeIO io, const char *executable_path) {
    const char *configured = getenv("LUME_APRENDER_DIR");
    char *near_executable = NULL;
    const char *directory = NULL;
    int result;
    if (configured != NULL && configured[0] != '\0' && directory_available(configured)) {
        directory = configured;
    } else if (directory_available("conteudo/aprender")) {
        directory = "conteudo/aprender";
    } else {
        near_executable = executable_content_directory(executable_path);
        if (near_executable != NULL && directory_available(near_executable)) directory = near_executable;
    }
    result = learn_cli_from_directory(argc, argv, io, directory);
    memory_free(near_executable);
    return result;
}

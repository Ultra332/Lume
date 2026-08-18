#include "repl.h"
#include <string.h>
#include "common.h"
#include "diagnostic.h"
#include "memory.h"
#include "session.h"
static char *read_line(FILE *input, size_t *length) {
    char *bytes = NULL; size_t count = 0U, capacity = 0U; int character;
    while ((character = fgetc(input)) != EOF) {
        char *grown; size_t next;
        if (count == capacity) { if (!memory_grow_capacity(capacity, count + 2U, &next)) { memory_free(bytes); return NULL; }
            grown = memory_reallocate_array(bytes, next, sizeof(*grown)); if (grown == NULL) { memory_free(bytes); return NULL; } bytes = grown; capacity = next; }
        bytes[count++] = (char)character; if (character == '\n') break;
    }
    if (character == EOF && count == 0U) { memory_free(bytes); *length = 0U; return NULL; }
    if (count == capacity) { char *grown = memory_reallocate_array(bytes, count + 1U, sizeof(*grown)); if (grown == NULL) { memory_free(bytes); return NULL; } bytes = grown; }
    bytes[count] = '\0'; *length = count; return bytes;
}
static bool append(char **buffer, size_t *length, size_t *capacity, const char *line, size_t line_length) {
    char *grown; size_t needed, next;
    if (*length > SIZE_MAX - line_length - 1U) return false;
    needed = *length + line_length + 1U;
    if (needed > *capacity) { if (!memory_grow_capacity(*capacity, needed, &next)) return false;
        grown = memory_reallocate_array(*buffer, next, sizeof(*grown)); if (grown == NULL) return false; *buffer = grown; *capacity = next; }
    memcpy(*buffer + *length, line, line_length); *length += line_length; (*buffer)[*length] = '\0'; return true;
}
static void repl_help(FILE *output) { fputs("Comandos:\n  :ajuda   mostra esta ajuda\n  :sair    encerra o Lume\n  :limpar  limpa a tela\n", output); }
int repl_run(RuntimeIO io) {
    LumeSession session; char *buffer = NULL; size_t length = 0U, capacity = 0U; bool continuation = false; size_t entry = 1U;
    session_init(&session, io); fprintf(io.output, "Lume %s\nModo interativo\nDigite :ajuda para obter ajuda.\n\n", LUME_VERSION_STRING);
    for (;;) {
        char *line; size_t line_length; Source *error_source = NULL; ErrorList errors; bool ok;
        fputs(continuation ? "... " : ">>> ", io.output); fflush(io.output);
        line = read_line(io.input, &line_length); if (line == NULL) { fputc('\n', io.output); break; }
        if (!continuation && line[0] == ':') {
            while (line_length > 0U && (line[line_length-1U] == '\n' || line[line_length-1U] == '\r')) line[--line_length] = '\0';
            if (strcmp(line, ":sair") == 0) { memory_free(line); break; }
            if (strcmp(line, ":ajuda") == 0) repl_help(io.output); else if (strcmp(line, ":limpar") == 0) fputs("\033[2J\033[H", io.output);
            else fprintf(io.output, "Comando desconhecido: %s\n", line);
            memory_free(line); continue;
        }
        if (!append(&buffer, &length, &capacity, line, line_length)) { memory_free(line); break; } memory_free(line);
        if (session_classify(buffer, length) == INPUT_INCOMPLETE) { continuation = true; continue; }
        continuation = false; error_list_init(&errors);
        { char name[64]; (void)snprintf(name, sizeof(name), "<repl:%zu>", entry++);
          ok = session_execute_repl(&session, name, buffer, length, &error_source, &errors); }
        if (!ok && errors.count > 0U) diagnostic_render(io.output, error_source, &errors.data[0]);
        if (!ok && error_source != NULL) { source_free(error_source); memory_free(error_source); }
        error_list_free(&errors); length = 0U; if (buffer != NULL) buffer[0] = '\0';
    }
    memory_free(buffer); session_free(&session); return 0;
}

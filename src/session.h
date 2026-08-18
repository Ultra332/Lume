#ifndef LUME_SESSION_H
#define LUME_SESSION_H
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "module.h"
typedef enum { INPUT_COMPLETE, INPUT_INCOMPLETE, INPUT_INVALID } InputStatus;
typedef struct {
    Environment environment;
    RuntimeIO io;
    ModuleRegistry modules;
    Program **programs; Source **sources; size_t retained_count; size_t retained_capacity;
} LumeSession;
void session_init(LumeSession *session, RuntimeIO io);
void session_free(LumeSession *session);
InputStatus session_classify(const char *text, size_t length);
bool session_execute(LumeSession *session, const char *name, const char *text, size_t length,
                     bool print_expression, Source **error_source, ErrorList *errors);
bool session_execute_repl(LumeSession *session, const char *name, const char *text, size_t length,
                          Source **error_source, ErrorList *errors);
#endif

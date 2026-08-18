#ifndef LUME_ERROR_H
#define LUME_ERROR_H
#include "common.h"
typedef struct { size_t offset; size_t line; size_t column; } SourceLocation;
typedef struct { SourceLocation start; SourceLocation end; } SourceSpan;
typedef enum { LUME_ERROR_NONE, LUME_ERROR_LEXICAL, LUME_ERROR_SYNTAX,
    LUME_ERROR_RUNTIME, LUME_ERROR_TYPE, LUME_ERROR_NUMERIC,
    LUME_ERROR_NAME, LUME_ERROR_ASSIGNMENT, LUME_ERROR_CALL, LUME_ERROR_CONVERSION, LUME_ERROR_INDEX,
    LUME_ERROR_MEMORY, LUME_ERROR_MODULE, LUME_ERROR_PROJECT, LUME_ERROR_DEPENDENCY, LUME_ERROR_INTERNAL } LumeErrorKind;
typedef struct { LumeErrorKind kind; SourceSpan span; const char *message;
    const char *suggestion; const char *subject; size_t subject_length; } LumeError;
typedef struct { LumeError *data; size_t count; size_t capacity; } ErrorList;
void error_list_init(ErrorList *list);
bool error_list_add(ErrorList *list, LumeError error);
void error_list_free(ErrorList *list);
const char *error_kind_name(LumeErrorKind kind);
#endif

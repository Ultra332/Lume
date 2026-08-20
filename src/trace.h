#ifndef LUME_TRACE_H
#define LUME_TRACE_H
#include "error.h"
#include "value.h"
typedef struct Environment Environment;
typedef enum {
    TRACE_PROGRAM_START, TRACE_PROGRAM_END, TRACE_DECLARE_VARIABLE,
    TRACE_DECLARE_CONSTANT, TRACE_DECLARE_FUNCTION, TRACE_ASSIGN,
    TRACE_IF_CONDITION, TRACE_WHILE_CONDITION, TRACE_WHILE_ITERATION,
    TRACE_WHILE_END, TRACE_FOR_START, TRACE_FOR_ITERATION, TRACE_FOR_END,
    TRACE_FOREACH_START, TRACE_FOREACH_END, TRACE_BREAK, TRACE_CONTINUE,
    TRACE_FUNCTION_CALL, TRACE_FUNCTION_ENTER, TRACE_FUNCTION_RETURN,
    TRACE_NATIVE_CALL, TRACE_OUTPUT, TRACE_LIST_CREATE, TRACE_INDEX_READ,
    TRACE_INDEX_WRITE, TRACE_LIST_APPEND, TRACE_LIST_REMOVE,
    TRACE_MODULE_IMPORT, TRACE_MODULE_LOADED
} TraceEventType;
/* Borrowed fields are valid only for the duration of the synchronous callback. */
typedef struct {
    TraceEventType type; SourceSpan span; const char *name; size_t name_length;
    const Value *before; const Value *after; const Value *arguments;
    size_t argument_count; const Environment *environment; bool decision;
    size_t iteration; size_t call_depth; int64_t index;
} TraceEvent;
typedef void (*TraceCallback)(void *context, const TraceEvent *event);
typedef struct { TraceCallback callback; void *context; bool stop_requested; } RuntimeTrace;
#endif

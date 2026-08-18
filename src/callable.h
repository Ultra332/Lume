#ifndef LUME_CALLABLE_H
#define LUME_CALLABLE_H
#include "common.h"
#include "value.h"
#include "error.h"
#include "runtime_io.h"
typedef struct Environment Environment;
typedef struct Stmt Stmt;
typedef enum {
    CALLABLE_USER, CALLABLE_NATIVE_WRITE, CALLABLE_NATIVE_READ,
    CALLABLE_NATIVE_TEXT, CALLABLE_NATIVE_INTEGER, CALLABLE_NATIVE_DECIMAL,
    CALLABLE_NATIVE_TYPE, CALLABLE_NATIVE_LENGTH, CALLABLE_NATIVE_APPEND, CALLABLE_NATIVE_REMOVE,
    CALLABLE_NATIVE_CUSTOM
} CallableType;
typedef bool (*NativeFunction)(const Value *arguments,size_t count,RuntimeIO *io,
    void *context,Value *out,SourceSpan span,ErrorList *errors);
typedef struct Callable {
    size_t references;
    CallableType type;
    const char *name;
    size_t arity;
    const Stmt *declaration;
    Environment *closure;
    NativeFunction native_function; void *native_context;
} Callable;
Callable *callable_new_user(const Stmt *declaration, Environment *closure);
Callable *callable_new_native(CallableType type, const char *name, size_t arity);
Callable *callable_new_custom(const char *name,size_t arity,NativeFunction function,void *context);
void callable_retain(Callable *callable);
void callable_release(Callable *callable);
#endif

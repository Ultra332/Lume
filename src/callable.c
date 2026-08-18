#include "callable.h"
#include "ast.h"
#include "memory.h"
Callable *callable_new_user(const Stmt *declaration, Environment *closure) {
    Callable *callable;
    if (declaration == NULL || declaration->type != STMT_FUNCTION) return NULL;
    callable = memory_allocate(sizeof(*callable));
    if (callable == NULL) return NULL;
    callable->references = 1U; callable->type = CALLABLE_USER;
    callable->name = declaration->as.function.name;
    callable->arity = declaration->as.function.parameter_count;
    callable->declaration = declaration; callable->closure = closure;callable->native_function=NULL;callable->native_context=NULL;
    return callable;
}
Callable *callable_new_native(CallableType type, const char *name, size_t arity) {
    Callable *callable = memory_allocate(sizeof(*callable));
    if (callable == NULL) return NULL;
    callable->references = 1U; callable->type = type; callable->name = name;
    callable->arity = arity; callable->declaration = NULL; callable->closure = NULL;callable->native_function=NULL;callable->native_context=NULL;
    return callable;
}
Callable *callable_new_custom(const char *name,size_t arity,NativeFunction function,void *context){Callable *c=callable_new_native(CALLABLE_NATIVE_CUSTOM,name,arity);if(c!=NULL){c->native_function=function;c->native_context=context;}return c;}
void callable_retain(Callable *callable) { if (callable != NULL) callable->references++; }
void callable_release(Callable *callable) {
    if (callable != NULL && --callable->references == 0U) memory_free(callable);
}

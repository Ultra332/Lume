#ifndef LUME_ENVIRONMENT_H
#define LUME_ENVIRONMENT_H
#include "error.h"
#include "value.h"

typedef struct Binding Binding;
typedef struct EnvironmentArena EnvironmentArena;
typedef struct Environment {
    Binding *entries;
    size_t count;
    size_t capacity;
    struct Environment *parent;
    bool is_global;
    EnvironmentArena *arena;
    bool arena_owned;
} Environment;

void environment_init(Environment *environment, Environment *parent);
void environment_free(Environment *environment);
Environment *environment_new_child(Environment *parent);
/* Preserva este ambiente e sua cadeia de pais para uma closure. */
bool environment_capture(Environment *environment);
/* Encerra um escopo filho; ambientes capturados permanecem na arena. */
void environment_release_child(Environment *environment);
/* Observabilidade interna usada pelos testes de ownership. */
size_t environment_retained_child_count(const Environment *environment);
bool environment_has_current(const Environment *environment, const char *name, size_t length);
bool environment_validate_definition(const Environment *environment, const char *name, size_t length,
                                     SourceSpan declaration_span, ErrorList *errors);
bool environment_define(Environment *environment, const char *name, size_t length,
                        const Value *value, bool mutable, SourceSpan declaration_span,
                        ErrorList *errors);
bool environment_define_native(Environment *environment, const char *name, size_t length,
                               const Value *value, SourceSpan span, ErrorList *errors);
bool environment_get(const Environment *environment, const char *name, size_t length,
                     Value *out, SourceSpan use_span, ErrorList *errors);
bool environment_assign(Environment *environment, const char *name, size_t length,
                        const Value *value, SourceSpan assignment_span, ErrorList *errors);
/* Runtime-only update for immutable bindings owned by control-flow machinery. */
bool environment_set_local_internal(Environment *environment, const char *name,
                                    size_t length, const Value *value);
typedef void (*EnvironmentBindingVisitor)(void *context, const char *name,
    size_t length, const Value *value, bool mutable);
void environment_visit_current(const Environment *environment,
    EnvironmentBindingVisitor visitor, void *context);

#endif

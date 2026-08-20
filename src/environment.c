#include "environment.h"

#include <stdint.h>
#include <string.h>
#include "memory.h"

struct Binding {
    char *name;
    size_t name_length;
    uint64_t hash;
    Value value;
    bool mutable;
    bool occupied;
    SourceSpan declaration_span;
};
struct EnvironmentArena { Environment **children; size_t count; size_t capacity; };

static uint64_t hash_name(const char *name, size_t length) {
    uint64_t hash = UINT64_C(1469598103934665603);
    size_t index;
    for (index = 0U; index < length; index++) {
        hash ^= (uint64_t)(unsigned char)name[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
static bool names_equal(const Binding *binding, const char *name, size_t length, uint64_t hash) {
    return binding->occupied && binding->hash == hash && binding->name_length == length &&
        memcmp(binding->name, name, length) == 0;
}
static Binding *find_slot(Binding *entries, size_t capacity,
                          const char *name, size_t length, uint64_t hash) {
    size_t index = (size_t)(hash % (uint64_t)capacity);
    for (;;) {
        Binding *binding = &entries[index];
        if (!binding->occupied || names_equal(binding, name, length, hash)) return binding;
        index = (index + 1U) % capacity;
    }
}
static const Binding *find_existing(const Environment *environment,
                                    const char *name, size_t length, uint64_t hash) {
    size_t index;
    size_t checked;
    if (environment->capacity == 0U) return NULL;
    index = (size_t)(hash % (uint64_t)environment->capacity);
    for (checked = 0U; checked < environment->capacity; checked++) {
        const Binding *binding = &environment->entries[index];
        if (!binding->occupied) return NULL;
        if (names_equal(binding, name, length, hash)) return binding;
        index = (index + 1U) % environment->capacity;
    }
    return NULL;
}
static bool grow(Environment *environment, size_t capacity) {
    Binding *entries;
    size_t index;
    entries = memory_reallocate_array(NULL, capacity, sizeof(*entries));
    if (entries == NULL) return false;
    for (index = 0U; index < capacity; index++) {
        entries[index].occupied = false;
        entries[index].name = NULL;
        entries[index].value = value_null();
    }
    for (index = 0U; index < environment->capacity; index++) {
        Binding *old = &environment->entries[index];
        if (old->occupied) {
            Binding *slot = find_slot(entries, capacity, old->name, old->name_length, old->hash);
            *slot = *old;
        }
    }
    memory_free(environment->entries);
    environment->entries = entries;
    environment->capacity = capacity;
    return true;
}
static bool ensure_capacity(Environment *environment) {
    size_t capacity;
    if (environment->count == SIZE_MAX) return false;
    if (environment->capacity != 0U &&
        environment->count + 1U <= environment->capacity - environment->capacity / 4U) return true;
    if (environment->capacity == 0U) capacity = 8U;
    else {
        if (environment->capacity > SIZE_MAX / 2U) return false;
        capacity = environment->capacity * 2U;
    }
    return grow(environment, capacity);
}
static bool is_native_name(const char *name, size_t length) {
    return (length == 7U && memcmp(name, "escreva", 7U) == 0) ||
        (length == 4U && memcmp(name, "leia", 4U) == 0) ||
        (length == 5U && memcmp(name, "texto", 5U) == 0) ||
        (length == 7U && memcmp(name, "inteiro", 7U) == 0) ||
        (length == 7U && memcmp(name, "decimal", 7U) == 0) ||
        (length == 4U && memcmp(name, "tipo", 4U) == 0) ||
        (length == 7U && memcmp(name, "tamanho", 7U) == 0) ||
        (length == 8U && memcmp(name, "adicione", 8U) == 0) ||
        (length == 6U && memcmp(name, "remova", 6U) == 0);
}
static bool environment_error(ErrorList *errors, LumeErrorKind kind, SourceSpan span,
                              const char *message, const char *suggestion) {
    LumeError error;
    error.kind = kind; error.span = span; error.message = message; error.suggestion = suggestion;
    error.subject = NULL; error.subject_length = 0U;
    (void)error_list_add(errors, error);
    return false;
}
static bool environment_name_error(ErrorList *errors, LumeErrorKind kind, SourceSpan span,
                                   const char *name, size_t length,
                                   const char *message, const char *suggestion) {
    LumeError error;
    error.kind = kind; error.span = span; error.message = message; error.suggestion = suggestion;
    error.subject = name; error.subject_length = length;
    (void)error_list_add(errors, error);
    return false;
}

void environment_init(Environment *environment, Environment *parent) {
    if (environment == NULL) return;
    environment->entries = NULL; environment->count = 0U; environment->capacity = 0U;
    environment->parent = parent; environment->is_global = parent == NULL;
    environment->arena_owned = false;
    if (parent != NULL) environment->arena = parent->arena;
    else {
        environment->arena = memory_allocate(sizeof(*environment->arena));
        if (environment->arena != NULL) {
            environment->arena->children = NULL; environment->arena->count = 0U;
            environment->arena->capacity = 0U;
        }
    }
}
static void free_bindings(Environment *environment) {
    size_t index;
    if (environment == NULL) return;
    for (index = 0U; index < environment->capacity; index++) {
        Binding *binding = &environment->entries[index];
        if (binding->occupied) {
            memory_free(binding->name);
            value_free(&binding->value);
        }
    }
    memory_free(environment->entries);
    environment->entries = NULL; environment->count = 0U; environment->capacity = 0U;
}
void environment_free(Environment *environment) {
    size_t index;
    EnvironmentArena *arena;
    if (environment == NULL) return;
    if (!environment->is_global) {
        if (!environment->arena_owned) free_bindings(environment);
        return;
    }
    arena = environment->arena;
    if (arena != NULL) {
        for (index = 0U; index < arena->count; index++) free_bindings(arena->children[index]);
        free_bindings(environment);
        for (index = 0U; index < arena->count; index++) memory_free(arena->children[index]);
        memory_free(arena->children); memory_free(arena);
    } else free_bindings(environment);
    environment->parent = NULL; environment->is_global = true;
    environment->arena = NULL; environment->arena_owned = false;
}
Environment *environment_new_child(Environment *parent) {
    Environment *child;
    if (parent == NULL || parent->arena == NULL) return NULL;
    child = memory_allocate(sizeof(*child));
    if (child == NULL) return NULL;
    environment_init(child, parent);
    return child;
}
bool environment_capture(Environment *environment) {
    Environment *current;
    if (environment == NULL || environment->arena == NULL) return false;
    for (current = environment; current != NULL && !current->is_global; current = current->parent) {
        Environment **grown;
        size_t capacity;
        if (current->arena_owned) continue;
        if (current->arena->count == current->arena->capacity) {
            if (!memory_grow_capacity(current->arena->capacity,
                    current->arena->count + 1U, &capacity)) return false;
            grown = memory_reallocate_array(current->arena->children,
                capacity, sizeof(*grown));
            if (grown == NULL) return false;
            current->arena->children = grown;
            current->arena->capacity = capacity;
        }
        current->arena->children[current->arena->count++] = current;
        current->arena_owned = true;
    }
    return true;
}
void environment_release_child(Environment *environment) {
    if (environment == NULL || environment->is_global || environment->arena_owned) return;
    free_bindings(environment);
    memory_free(environment);
}
size_t environment_retained_child_count(const Environment *environment) {
    return environment == NULL || environment->arena == NULL ? 0U : environment->arena->count;
}
bool environment_has_current(const Environment *environment, const char *name, size_t length) {
    uint64_t hash;
    if (environment == NULL || name == NULL) return false;
    hash = hash_name(name, length);
    return find_existing(environment, name, length, hash) != NULL;
}
bool environment_validate_definition(const Environment *environment, const char *name, size_t length,
                                     SourceSpan declaration_span, ErrorList *errors) {
    if (environment == NULL || name == NULL || errors == NULL) return false;
    if (environment->is_global && is_native_name(name, length))
        return environment_name_error(errors, LUME_ERROR_RUNTIME, declaration_span, name, length,
            "Este nome e reservado para uma funcao nativa no escopo global.",
            "Escolha outro nome para a variavel ou constante.");
    if (environment_has_current(environment, name, length))
        return environment_name_error(errors, LUME_ERROR_RUNTIME, declaration_span, name, length,
            "Este nome ja foi declarado neste escopo.",
            "Escolha outro nome ou atribua ao binding existente.");
    return true;
}
bool environment_define(Environment *environment, const char *name, size_t length,
                        const Value *value, bool mutable, SourceSpan declaration_span,
                        ErrorList *errors) {
    uint64_t hash;
    Binding *slot;
    char *name_copy;
    Value value_copy_item = value_null();
    if (environment == NULL || name == NULL || value == NULL || errors == NULL) return false;
    if (!environment_validate_definition(environment, name, length, declaration_span, errors))
        return false;
    hash = hash_name(name, length);
    if (!ensure_capacity(environment))
        return environment_error(errors, LUME_ERROR_MEMORY, declaration_span,
            "Nao foi possivel ampliar o ambiente.", "Tente um programa menor.");
    name_copy = memory_copy_string(name, length);
    if (name_copy == NULL || !value_copy(value, &value_copy_item)) {
        memory_free(name_copy);
        return environment_error(errors, LUME_ERROR_MEMORY, declaration_span,
            "Nao foi possivel copiar o binding.", "Tente um valor menor.");
    }
    slot = find_slot(environment->entries, environment->capacity, name, length, hash);
    slot->name = name_copy; slot->name_length = length; slot->hash = hash;
    slot->value = value_copy_item; slot->mutable = mutable; slot->occupied = true;
    slot->declaration_span = declaration_span; environment->count++;
    return true;
}
bool environment_define_native(Environment *environment, const char *name, size_t length,
                               const Value *value, SourceSpan span, ErrorList *errors) {
    bool was_global;
    bool ok;
    if (environment == NULL) return false;
    was_global = environment->is_global; environment->is_global = false;
    ok = environment_define(environment, name, length, value, false, span, errors);
    environment->is_global = was_global;
    return ok;
}
bool environment_get(const Environment *environment, const char *name, size_t length,
                     Value *out, SourceSpan use_span, ErrorList *errors) {
    const Environment *current = environment;
    uint64_t hash;
    if (name == NULL || out == NULL || errors == NULL) return false;
    hash = hash_name(name, length);
    while (current != NULL) {
        const Binding *binding = find_existing(current, name, length, hash);
        if (binding != NULL) {
            if (!value_copy(&binding->value, out))
                return environment_error(errors, LUME_ERROR_MEMORY, use_span,
                    "Nao foi possivel copiar o valor da variavel.", "Tente um valor menor.");
            return true;
        }
        current = current->parent;
    }
    return environment_name_error(errors, LUME_ERROR_NAME, use_span, name, length,
        "A variavel usada nao foi definida.", "Declare-a com 'variavel nome = valor' antes de usa-la.");
}
bool environment_assign(Environment *environment, const char *name, size_t length,
                        const Value *value, SourceSpan assignment_span, ErrorList *errors) {
    Environment *current = environment;
    uint64_t hash;
    if (name == NULL || value == NULL || errors == NULL) return false;
    hash = hash_name(name, length);
    while (current != NULL) {
        Binding *binding = (Binding *)find_existing(current, name, length, hash);
        if (binding != NULL) {
            Value copy = value_null();
            if (!binding->mutable)
                return environment_name_error(errors, LUME_ERROR_ASSIGNMENT, assignment_span, name, length,
                    "Nao e possivel alterar uma constante.", "Remova a atribuicao ou use uma variavel.");
            if (!value_copy(value, &copy))
                return environment_error(errors, LUME_ERROR_MEMORY, assignment_span,
                    "Nao foi possivel copiar o novo valor.", "Tente um valor menor.");
            value_free(&binding->value); binding->value = copy;
            return true;
        }
        current = current->parent;
    }
    return environment_name_error(errors, LUME_ERROR_NAME, assignment_span, name, length,
        "Nao e possivel atribuir a um nome que nao foi definido.",
        "Declare a variavel antes da atribuicao.");
}
bool environment_set_local_internal(Environment *environment, const char *name,
                                    size_t length, const Value *value) {
    uint64_t hash;
    Binding *binding;
    Value copy = value_null();
    if (environment == NULL || name == NULL || value == NULL) return false;
    hash = hash_name(name, length);
    binding = (Binding *)find_existing(environment, name, length, hash);
    if (binding == NULL || !value_copy(value, &copy)) return false;
    value_free(&binding->value);
    binding->value = copy;
    return true;
}
void environment_visit_current(const Environment *environment,
    EnvironmentBindingVisitor visitor, void *context) {
    size_t index;
    if (environment == NULL || visitor == NULL) return;
    for (index = 0U; index < environment->capacity; index++) {
        const Binding *binding = &environment->entries[index];
        if (binding->occupied)
            visitor(context, binding->name, binding->name_length,
                    &binding->value, binding->mutable);
    }
}

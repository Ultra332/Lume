#include "error.h"
#include "memory.h"
void error_list_init(ErrorList *list) {
    if (list != NULL) { list->data = NULL; list->count = 0U; list->capacity = 0U; }
}
bool error_list_add(ErrorList *list, LumeError error) {
    LumeError *grown; size_t capacity;
    if (list == NULL) return false;
    if (list->count == list->capacity) {
        if (list->count == SIZE_MAX) return false;
        if (!memory_grow_capacity(list->capacity, list->count + 1U, &capacity)) return false;
        grown = memory_reallocate_array(list->data, capacity, sizeof(*grown));
        if (grown == NULL) return false;
        list->data = grown; list->capacity = capacity;
    }
    list->data[list->count++] = error;
    return true;
}
void error_list_free(ErrorList *list) {
    if (list != NULL) { memory_free(list->data); error_list_init(list); }
}
const char *error_kind_name(LumeErrorKind kind) {
    switch (kind) {
        case LUME_ERROR_LEXICAL: return "Erro lexico";
        case LUME_ERROR_SYNTAX: return "Erro de sintaxe";
        case LUME_ERROR_RUNTIME: return "Erro de execucao";
        case LUME_ERROR_TYPE: return "Erro de tipo";
        case LUME_ERROR_NUMERIC: return "Erro numerico";
        case LUME_ERROR_NAME: return "Erro de nome";
        case LUME_ERROR_ASSIGNMENT: return "Erro de atribuicao";
        case LUME_ERROR_CALL: return "Erro de chamada";
        case LUME_ERROR_CONVERSION: return "Erro de conversao";
        case LUME_ERROR_INDEX: return "Erro de indice";
        case LUME_ERROR_MEMORY: return "Erro de memoria";
        case LUME_ERROR_MODULE: return "Erro de modulo";
        case LUME_ERROR_PROJECT: return "Erro de projeto";
        case LUME_ERROR_DEPENDENCY: return "Erro de dependencia";
        case LUME_ERROR_INTERNAL: return "Erro interno";
        case LUME_ERROR_NONE: return "Sem erro";
    }
    return "Erro desconhecido";
}

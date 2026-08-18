#include "error.h"
#include "memory.h"
void error_list_init(ErrorList *list) {
    if (list != NULL) { list->data = NULL; list->count = 0U; list->capacity = 0U; }
}
bool error_list_add(ErrorList *list, LumeError error) {
    LumeError *grown; size_t capacity; char *subject_copy = NULL;
    if (list == NULL) return false;
    if (list->count == list->capacity) {
        if (list->count == SIZE_MAX) return false;
        if (!memory_grow_capacity(list->capacity, list->count + 1U, &capacity)) return false;
        grown = memory_reallocate_array(list->data, capacity, sizeof(*grown));
        if (grown == NULL) return false;
        list->data = grown; list->capacity = capacity;
    }
    /* 'subject' costuma apontar para dentro da AST, que pode ser liberada antes
       do diagnostico ser exibido. A lista guarda uma copia propria. Se a copia
       falhar, o erro ainda e registrado sem o nome: perder o diagnostico inteiro
       seria pior do que perder um detalhe dele. */
    if (error.subject != NULL && error.subject_length > 0U) {
        subject_copy = memory_copy_string(error.subject, error.subject_length);
    }
    if (subject_copy == NULL) { error.subject = NULL; error.subject_length = 0U; }
    else { error.subject = subject_copy; }
    list->data[list->count++] = error;
    return true;
}
void error_list_free(ErrorList *list) {
    size_t index;
    if (list == NULL) return;
    for (index = 0U; index < list->count; index++) {
        /* A copia foi feita em error_list_add; o const descreve o uso, nao a posse. */
        memory_free((char *)list->data[index].subject);
        list->data[index].subject = NULL; list->data[index].subject_length = 0U;
    }
    memory_free(list->data); error_list_init(list);
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

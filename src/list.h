#ifndef LUME_LIST_H
#define LUME_LIST_H
#include "value.h"
struct LumeList { size_t references; Value *items; size_t count; size_t capacity; };
LumeList *list_new(void);
void list_retain(LumeList *list);
void list_release(LumeList *list);
bool list_append(LumeList *list, const Value *value);
bool list_get(const LumeList *list, size_t index, Value *out);
bool list_set(LumeList *list, size_t index, const Value *value);
bool list_remove(LumeList *list, size_t index, Value *out);
bool list_would_create_cycle(const LumeList *target, const Value *value);
bool list_format(const LumeList *list, char **out, size_t *length);
#endif

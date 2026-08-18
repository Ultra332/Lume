#ifndef LUME_VALUE_H
#define LUME_VALUE_H
#include <stdio.h>
#include "common.h"
typedef struct Callable Callable;
typedef struct LumeList LumeList;
typedef struct LumeModule LumeModule;
typedef enum { VALUE_NULL, VALUE_BOOLEAN, VALUE_INTEGER, VALUE_DECIMAL, VALUE_STRING, VALUE_CALLABLE, VALUE_LIST, VALUE_MODULE } ValueType;
typedef struct { char *bytes; size_t length; } LumeString;
typedef struct Value {
    ValueType type;
    union { bool boolean; int64_t integer; double decimal; LumeString string; Callable *callable; LumeList *list; LumeModule *module; } as;
} Value;
Value value_null(void);
Value value_boolean(bool value);
Value value_integer(int64_t value);
Value value_decimal(double value);
Value value_callable(Callable *callable);
Value value_list(LumeList *list);
Value value_module(LumeModule *module);
bool value_string_copy(const char *bytes, size_t length, Value *out);
bool value_string_decode(const char *quoted, size_t length, Value *out);
bool value_copy(const Value *source, Value *out);
void value_free(Value *value);
const char *value_type_name(ValueType type);
void value_print(FILE *stream, const Value *value);
bool value_format(const Value *value, char **out, size_t *length);
bool value_format_nested(const Value *value, char **out, size_t *length);
#endif

#include "value.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "callable.h"
#include "list.h"
#include "module.h"

Value value_null(void) { Value value; value.type = VALUE_NULL; value.as.integer = 0; return value; }
Value value_boolean(bool item) { Value value; value.type = VALUE_BOOLEAN; value.as.boolean = item; return value; }
Value value_integer(int64_t item) { Value value; value.type = VALUE_INTEGER; value.as.integer = item; return value; }
Value value_decimal(double item) { Value value; value.type = VALUE_DECIMAL; value.as.decimal = item; return value; }
Value value_callable(Callable *item) { Value value; value.type = VALUE_CALLABLE; value.as.callable = item; return value; }
Value value_list(LumeList *item) { Value value; value.type = VALUE_LIST; value.as.list = item; return value; }
Value value_module(LumeModule *item) { Value value; value.type=VALUE_MODULE;value.as.module=item;return value; }
bool value_string_copy(const char *bytes, size_t length, Value *out) {
    char *copy;
    if (out == NULL || (bytes == NULL && length != 0U)) return false;
    copy = memory_copy_string(bytes == NULL ? "" : bytes, length);
    if (copy == NULL) return false;
    out->type = VALUE_STRING; out->as.string.bytes = copy; out->as.string.length = length;
    return true;
}
bool value_string_decode(const char *quoted, size_t length, Value *out) {
    char *decoded;
    size_t read_at;
    size_t write_at = 0U;
    if (quoted == NULL || out == NULL || length < 2U || quoted[0] != '"' || quoted[length - 1U] != '"')
        return false;
    decoded = memory_allocate(length - 1U);
    if (decoded == NULL) return false;
    for (read_at = 1U; read_at + 1U < length; read_at++) {
        unsigned char current = (unsigned char)quoted[read_at];
        if (current == (unsigned char)'\\') {
            unsigned char escaped;
            read_at++;
            if (read_at + 1U > length) { memory_free(decoded); return false; }
            escaped = (unsigned char)quoted[read_at];
            switch (escaped) {
                case 'n': current = (unsigned char)'\n'; break;
                case 'r': current = (unsigned char)'\r'; break;
                case 't': current = (unsigned char)'\t'; break;
                case '"': current = (unsigned char)'"'; break;
                case '\\': current = (unsigned char)'\\'; break;
                default: memory_free(decoded); return false;
            }
        }
        decoded[write_at++] = (char)current;
    }
    decoded[write_at] = '\0';
    out->type = VALUE_STRING; out->as.string.bytes = decoded; out->as.string.length = write_at;
    return true;
}
bool value_copy(const Value *source, Value *out) {
    if (source == NULL || out == NULL) return false;
    if (source->type == VALUE_STRING)
        return value_string_copy(source->as.string.bytes, source->as.string.length, out);
    *out = *source;
    if (source->type == VALUE_CALLABLE) callable_retain(source->as.callable);
    if (source->type == VALUE_LIST) list_retain(source->as.list);
    return true;
}
void value_free(Value *value) {
    if (value != NULL) {
        if (value->type == VALUE_STRING) memory_free(value->as.string.bytes);
        if (value->type == VALUE_CALLABLE) callable_release(value->as.callable);
        if (value->type == VALUE_LIST) list_release(value->as.list);
        *value = value_null();
    }
}
const char *value_type_name(ValueType type) {
    switch (type) {
        case VALUE_NULL: return "nulo";
        case VALUE_BOOLEAN: return "booleano";
        case VALUE_INTEGER: return "inteiro";
        case VALUE_DECIMAL: return "decimal";
        case VALUE_STRING: return "texto";
        case VALUE_CALLABLE: return "funcao";
        case VALUE_LIST: return "lista";
        case VALUE_MODULE: return "modulo";
    }
    return "desconhecido";
}
void value_print(FILE *stream, const Value *value) {
    char *text; size_t length;
    if (stream == NULL || value == NULL || !value_format(value, &text, &length)) return;
    fwrite(text, 1U, length, stream); memory_free(text);
}
bool value_format(const Value *value, char **out, size_t *length) {
    char buffer[128]; int written; const char *text = NULL; size_t size = 0U;
    if (value == NULL || out == NULL || length == NULL) return false;
    switch (value->type) {
        case VALUE_NULL: text = "nulo"; size = 4U; break;
        case VALUE_BOOLEAN: text = value->as.boolean ? "verdadeiro" : "falso"; size = value->as.boolean ? 10U : 5U; break;
        case VALUE_STRING: text = value->as.string.bytes; size = value->as.string.length; break;
        case VALUE_INTEGER: written = snprintf(buffer, sizeof(buffer), "%" PRId64, value->as.integer); if (written < 0) return false; text = buffer; size = (size_t)written; break;
        case VALUE_DECIMAL: written = snprintf(buffer, sizeof(buffer), "%.15g", value->as.decimal); if (written < 0) return false; text = buffer; size = (size_t)written; break;
        case VALUE_CALLABLE: {
            size_t name_length = strlen(value->as.callable->name);
            char *formatted;
            if (name_length > SIZE_MAX - 10U) return false;
            formatted = memory_allocate(name_length + 10U); if (formatted == NULL) return false;
            memcpy(formatted, "<funcao ", 8U); memcpy(formatted + 8U, value->as.callable->name, name_length);
            formatted[8U + name_length] = '>'; formatted[9U + name_length] = '\0';
            *out = formatted; *length = name_length + 9U; return true;
        }
        case VALUE_LIST: return list_format(value->as.list, out, length);
        case VALUE_MODULE: {
            const char *name=value->as.module->name;size_t name_length=strlen(name);
            char *module_text=memory_allocate(name_length+11U);if(module_text==NULL)return false;
            memcpy(module_text,"<modulo ",8U);memcpy(module_text+8U,name,name_length);module_text[8U+name_length]='>';module_text[9U+name_length]='\0';*out=module_text;*length=9U+name_length;return true;
        }
    }
    *out = memory_copy_string(text, size); if (*out == NULL) return false; *length = size; return true;
}
bool value_format_nested(const Value *value, char **out, size_t *length) {
    char *formatted; size_t read_at, write_at = 0U, capacity;
    if (value == NULL || out == NULL || length == NULL) return false;
    if (value->type != VALUE_STRING) return value_format(value, out, length);
    if (value->as.string.length > (SIZE_MAX - 3U) / 2U) return false;
    capacity = value->as.string.length * 2U + 3U;
    formatted = memory_allocate(capacity); if (formatted == NULL) return false;
    formatted[write_at++] = '"';
    for (read_at = 0U; read_at < value->as.string.length; read_at++) {
        char character = value->as.string.bytes[read_at];
        if (character == '"' || character == '\\') { formatted[write_at++]='\\'; formatted[write_at++]=character; }
        else if (character == '\n' || character == '\r' || character == '\t') { formatted[write_at++]='\\'; formatted[write_at++]=character=='\n'?'n':(character=='\r'?'r':'t'); }
        else formatted[write_at++]=character;
    }
    formatted[write_at++]='"'; formatted[write_at]='\0'; *out=formatted; *length=write_at; return true;
}

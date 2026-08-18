#include "memory.h"
#include <stdlib.h>
#include <string.h>

bool memory_size_multiply(size_t left, size_t right, size_t *result) {
    if (result == NULL || (right != 0U && left > SIZE_MAX / right)) return false;
    *result = left * right;
    return true;
}
bool memory_grow_capacity(size_t current, size_t minimum, size_t *result) {
    size_t capacity = current < 8U ? 8U : current;
    if (result == NULL) return false;
    while (capacity < minimum) {
        if (capacity > SIZE_MAX / 2U) { capacity = minimum; break; }
        capacity *= 2U;
    }
    *result = capacity;
    return true;
}
void *memory_allocate(size_t size) { return malloc(size == 0U ? 1U : size); }
void *memory_reallocate_array(void *pointer, size_t count, size_t item_size) {
    size_t size;
    if (!memory_size_multiply(count, item_size, &size)) return NULL;
    return realloc(pointer, size == 0U ? 1U : size);
}
void memory_free(void *pointer) { free(pointer); }
char *memory_copy_string(const char *bytes, size_t length) {
    char *copy;
    if (bytes == NULL || length == SIZE_MAX) return NULL;
    copy = memory_allocate(length + 1U);
    if (copy == NULL) return NULL;
    if (length > 0U) memcpy(copy, bytes, length);
    copy[length] = '\0';
    return copy;
}

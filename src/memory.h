#ifndef LUME_MEMORY_H
#define LUME_MEMORY_H
#include "common.h"
bool memory_size_multiply(size_t left, size_t right, size_t *result);
bool memory_grow_capacity(size_t current, size_t minimum, size_t *result);
void *memory_allocate(size_t size);
void *memory_reallocate_array(void *pointer, size_t count, size_t item_size);
void memory_free(void *pointer);
char *memory_copy_string(const char *bytes, size_t length);
#endif

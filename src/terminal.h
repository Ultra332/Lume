#ifndef LUME_TERMINAL_H
#define LUME_TERMINAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool terminal_clear(FILE *output);
bool terminal_position(FILE *output, size_t column, size_t row);
bool terminal_cursor(FILE *output, bool visible);
void terminal_size(FILE *output, size_t *columns, size_t *rows);
bool terminal_read_key(FILE *input, bool blocking, int *character, bool *available);

#endif

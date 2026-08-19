#ifndef LUME_SOURCE_H
#define LUME_SOURCE_H
#include "common.h"
/* Source owns name and bytes until source_free(). */
typedef struct Source { char *name; char *bytes; size_t length; } Source;
void source_init(Source *source);
bool source_from_bytes(Source *source, const char *name, const char *bytes, size_t length);
bool source_load_file(Source *source, const char *path);
void source_free(Source *source);
#endif

#include "source.h"
#include <stdio.h>
#include <string.h>
#include "memory.h"

void source_init(Source *source) {
    if (source != NULL) { source->name = NULL; source->bytes = NULL; source->length = 0U; }
}
void source_free(Source *source) {
    if (source != NULL) { memory_free(source->name); memory_free(source->bytes); source_init(source); }
}
bool source_from_bytes(Source *source, const char *name, const char *bytes, size_t length) {
    char *name_copy;
    char *bytes_copy;
    if (source == NULL || name == NULL || (bytes == NULL && length != 0U)) return false;
    name_copy = memory_copy_string(name, strlen(name));
    bytes_copy = memory_copy_string(bytes == NULL ? "" : bytes, length);
    if (name_copy == NULL || bytes_copy == NULL) {
        memory_free(name_copy); memory_free(bytes_copy); return false;
    }
    source_free(source);
    source->name = name_copy; source->bytes = bytes_copy; source->length = length;
    return true;
}
bool source_load_file(Source *source, const char *path) {
    FILE *file;
    long end;
    size_t length;
    size_t read_count;
    char *bytes;
    bool ok;
    if (source == NULL || path == NULL) return false;
    file = fopen(path, "rb");
    if (file == NULL) return false;
    if (fseek(file, 0L, SEEK_END) != 0) { fclose(file); return false; }
    end = ftell(file);
    if (end < 0L || (uintmax_t)end > (uintmax_t)SIZE_MAX || fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file); return false;
    }
    length = (size_t)end;
    if (length == SIZE_MAX) { fclose(file); return false; }
    bytes = memory_allocate(length + 1U);
    if (bytes == NULL) { fclose(file); return false; }
    read_count = length == 0U ? 0U : fread(bytes, 1U, length, file);
    if (fclose(file) != 0 || read_count != length) { memory_free(bytes); return false; }
    bytes[length] = '\0';
    ok = source_from_bytes(source, path, bytes, length);
    memory_free(bytes);
    return ok;
}

#ifndef LUME_STDLIB_H
#define LUME_STDLIB_H
#include "module.h"
bool stdlib_is_path(const char *path,size_t length);
bool stdlib_create_module(ModuleRegistry *registry,const char *importer,const char *path,
    size_t length,LumeModule **out,ErrorList *errors,SourceSpan span);
bool stdlib_has_export(const char *path,size_t path_length,const char *name,size_t name_length);
#endif

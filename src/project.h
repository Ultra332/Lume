#ifndef LUME_PROJECT_H
#define LUME_PROJECT_H
#include "common.h"
typedef struct { char *name,*declared_path,*resolved_path,*version; size_t target_index; } ProjectDependency;
typedef struct { char *root_path,*manifest_path,*name,*version,*entry_path,*source_path;
    char **module_paths; size_t module_path_count; ProjectDependency *dependencies;
    size_t dependency_count; char message[512]; } LumeProject;
void project_init(LumeProject *project);
void project_free(LumeProject *project);
bool project_load(LumeProject *project,const char *root_path);
bool project_create(const char *parent_path,const char *name,char *message,size_t capacity);
#endif

#ifndef LUME_MODULE_H
#define LUME_MODULE_H
#include "ast.h"
#include "environment.h"
#include "runtime_io.h"
#include "source.h"
#include "trace.h"
typedef enum { MODULE_LOADING, MODULE_LOADED, MODULE_FAILED } ModuleState;
struct DependencyGraph;
typedef struct LumeModule {
    char *path; char *name; Source source; Program *program; Environment environment;
    ModuleState state; char **exports; size_t *export_lengths; size_t export_count;
    void *native_context;
} LumeModule;
typedef struct ModuleRegistry {
    LumeModule **modules; size_t count,capacity; RuntimeIO *io; RuntimeTrace *trace;
    const Source *error_source;
    const char *project_source_path; char *const *project_module_paths;
    size_t project_module_path_count;
    const struct DependencyGraph *dependency_graph;
} ModuleRegistry;
void module_registry_init(ModuleRegistry *registry,RuntimeIO *io,RuntimeTrace *trace);
void module_registry_free(ModuleRegistry *registry);
void module_registry_set_project(ModuleRegistry *registry,const char *source_path,
    char *const *module_paths,size_t module_path_count);
void module_registry_set_dependencies(ModuleRegistry *registry,const struct DependencyGraph *graph);
bool module_registry_import(ModuleRegistry *registry,const char *importer_path,
    const Stmt *statement,LumeModule **out,ErrorList *errors);
bool module_get_export(LumeModule *module,const char *name,size_t length,Value *out,
    SourceSpan span,ErrorList *errors);
bool module_registry_validate_program(ModuleRegistry *registry,const char *importer_path,
    const Source *source,const Program *program,ErrorList *errors);
#endif

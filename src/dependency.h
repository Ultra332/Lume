#ifndef LUME_DEPENDENCY_H
#define LUME_DEPENDENCY_H
#include "project.h"
typedef enum { DEPENDENCY_UNVISITED, DEPENDENCY_VISITING, DEPENDENCY_RESOLVED } DependencyState;
typedef struct { LumeProject project; DependencyState state; } DependencyNode;
typedef struct DependencyGraph { DependencyNode *nodes; size_t count,capacity; size_t root_index; char message[1024]; } DependencyGraph;
void dependency_graph_init(DependencyGraph *graph);
void dependency_graph_free(DependencyGraph *graph);
bool dependency_graph_resolve(DependencyGraph *graph,const char *root_path);
const LumeProject *dependency_graph_root(const DependencyGraph *graph);
bool dependency_graph_resolve_import(const DependencyGraph *graph,const char *importer,
    const char *requested,size_t requested_length,char **out);
bool dependency_lock_write(const DependencyGraph *graph,char *message,size_t capacity);
bool dependency_lock_current(const DependencyGraph *graph,bool *exists);
#endif

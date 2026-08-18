#ifndef LUME_EDUCATION_H
#define LUME_EDUCATION_H
#include "environment.h"
#include "runtime_io.h"
#include "source.h"
#include "trace.h"
typedef enum { EDUCATION_EXPLAIN, EDUCATION_STEP } EducationMode;
typedef struct {
    RuntimeIO *io; const Source *source; RuntimeTrace *trace; EducationMode mode;
    size_t step; size_t rendered; size_t omitted; bool continuing;
    const Environment *current_environment;
    const char *stack_names[64]; size_t stack_lengths[64]; size_t stack_count;
} EducationRenderer;
void education_renderer_init(EducationRenderer *renderer, RuntimeIO *io,
    const Source *source, RuntimeTrace *trace, EducationMode mode);
void education_trace_callback(void *context, const TraceEvent *event);
void education_show_variables(FILE *output, const Environment *environment);
int education_run_file(const char *path, RuntimeIO io, EducationMode mode);
#endif

#ifndef LUME_EDUCATION_H
#define LUME_EDUCATION_H
#include "environment.h"
#include "runtime_io.h"
#include "source.h"
#include "trace.h"
typedef enum { EDUCATION_EXPLAIN, EDUCATION_STEP } EducationMode;
enum { EDUCATION_STACK_LIMIT = 200, EDUCATION_NAME_LIMIT = 63 };
typedef struct {
    RuntimeIO *io; const Source *source; RuntimeTrace *trace; EducationMode mode;
    size_t step; size_t rendered; size_t omitted; size_t program_depth;
    bool continuing;
    const Environment *current_environment;
    char stack_names[EDUCATION_STACK_LIMIT][EDUCATION_NAME_LIMIT + 1];
    size_t stack_lengths[EDUCATION_STACK_LIMIT]; size_t stack_count;
} EducationRenderer;
void education_renderer_init(EducationRenderer *renderer, RuntimeIO *io,
    const Source *source, RuntimeTrace *trace, EducationMode mode);
void education_trace_callback(void *context, const TraceEvent *event);
void education_show_variables(FILE *output, const Environment *environment);
void education_print_value(FILE *output, const Value *value);
int education_run_file(const char *path, RuntimeIO io, EducationMode mode);
#endif

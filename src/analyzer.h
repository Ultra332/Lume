#ifndef LUME_ANALYZER_H
#define LUME_ANALYZER_H
#include <stdio.h>
#include "ast.h"
#include "source.h"
typedef enum { ANALYSIS_ERROR, ANALYSIS_WARNING, ANALYSIS_INFORMATION } AnalysisSeverity;
typedef enum {
    ANALYSIS_UNUSED_VARIABLE, ANALYSIS_UNUSED_CONSTANT, ANALYSIS_UNUSED_PARAMETER,
    ANALYSIS_UNUSED_FUNCTION, ANALYSIS_UNUSED_ITERATOR, ANALYSIS_ONLY_ASSIGNED,
    ANALYSIS_OVERWRITTEN_VALUE, ANALYSIS_UNREACHABLE, ANALYSIS_CONSTANT_CONDITION,
    ANALYSIS_EMPTY_FOR, ANALYSIS_SHADOWING, ANALYSIS_NATIVE_SHADOWING,
    ANALYSIS_UNDEFINED_NAME, ANALYSIS_SELF_ASSIGNMENT
} AnalysisCode;
typedef struct {
    AnalysisSeverity severity; AnalysisCode code; SourceSpan span;
    char *name; size_t name_length; char *suggestion; size_t suggestion_length;
    bool boolean_value;
} AnalysisDiagnostic;
typedef struct {
    size_t variables, constants, functions, conditions, while_loops, for_loops, lists;
} AnalysisMetrics;
typedef struct {
    AnalysisDiagnostic *diagnostics; size_t count, capacity;
    size_t errors, warnings, information; AnalysisMetrics metrics;
} AnalysisResult;
void analysis_result_init(AnalysisResult *result);
void analysis_result_free(AnalysisResult *result);
bool analyzer_analyze(const Program *program, AnalysisResult *result);
void analyzer_render(FILE *output, const Source *source, const AnalysisResult *result);
#endif

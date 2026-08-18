#ifndef LUME_INTERPRETER_H
#define LUME_INTERPRETER_H
#include "ast.h"
#include "environment.h"
#include "error.h"
#include "runtime_io.h"
#include "trace.h"
typedef struct ModuleRegistry ModuleRegistry;
typedef struct LumeModule LumeModule;

/* Evaluates expression AST only. Caller owns the successful output Value. */
bool interpreter_evaluate_expression(const Expr *expression, Value *out, ErrorList *errors);
bool interpreter_evaluate_expression_in_environment(const Expr *expression,
    Environment *environment, Value *out, ErrorList *errors);
bool interpreter_evaluate_expression_with_io(const Expr *expression, Environment *environment,
    RuntimeIO *io, Value *out, ErrorList *errors);
bool interpreter_execute_program(const Program *program, Environment *environment,
                                 ErrorList *errors);
bool interpreter_execute_program_with_io(const Program *program, Environment *environment,
                                         RuntimeIO *io, ErrorList *errors);
bool interpreter_execute_program_with_trace(const Program *program, Environment *environment,
    RuntimeIO *io, RuntimeTrace *trace, ErrorList *errors);
bool interpreter_execute_program_with_modules(const Program *program,Environment *environment,
    RuntimeIO *io,RuntimeTrace *trace,ModuleRegistry *registry,LumeModule *module,
    ErrorList *errors);

#endif

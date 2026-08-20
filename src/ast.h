#ifndef LUME_AST_H
#define LUME_AST_H
#include "error.h"
#include "value.h"
typedef enum { EXPR_LITERAL, EXPR_IDENTIFIER, EXPR_UNARY, EXPR_BINARY, EXPR_GROUPING, EXPR_CALL, EXPR_LIST, EXPR_INDEX, EXPR_MEMBER } ExprType;
typedef enum { UNARY_POSITIVE, UNARY_NEGATIVE, UNARY_NOT } UnaryOperator;
typedef enum {
    BINARY_ADD, BINARY_SUBTRACT, BINARY_MULTIPLY, BINARY_DIVIDE, BINARY_REMAINDER,
    BINARY_EQUAL, BINARY_NOT_EQUAL, BINARY_LESS, BINARY_LESS_EQUAL,
    BINARY_GREATER, BINARY_GREATER_EQUAL, BINARY_LOGICAL_AND, BINARY_LOGICAL_OR
} BinaryOperator;
typedef struct Expr Expr;
struct Expr {
    ExprType type;
    SourceSpan span;
    union {
        Value literal;
        struct { char *name; size_t length; } identifier;
        struct { UnaryOperator operator_type; SourceSpan operator_span; Expr *operand; } unary;
        struct { Expr *left; BinaryOperator operator_type; SourceSpan operator_span; Expr *right; } binary;
        struct { Expr *expression; } grouping;
        struct { Expr *callee; Expr **arguments; size_t argument_count; } call;
        struct { Expr **elements; size_t count; } list;
        struct { Expr *target; Expr *index; } index;
        struct { Expr *target; char *name; size_t name_length; SourceSpan name_span; } member;
    } as;
};
Expr *expr_new_literal(Value value, SourceSpan span);
Expr *expr_new_identifier(const char *name, size_t length, SourceSpan span);
Expr *expr_new_unary(UnaryOperator operator_type, SourceSpan operator_span, Expr *operand);
Expr *expr_new_binary(Expr *left, BinaryOperator operator_type, SourceSpan operator_span, Expr *right);
Expr *expr_new_grouping(Expr *expression, SourceSpan span);
Expr *expr_new_call(Expr *callee, Expr **arguments, size_t argument_count, SourceSpan span);
Expr *expr_new_list(Expr **elements, size_t count, SourceSpan span);
Expr *expr_new_index(Expr *target, Expr *index, SourceSpan span);
Expr *expr_new_member(Expr *target, const char *name, size_t length,
                      SourceSpan name_span, SourceSpan span);
void expr_free(Expr *expression);

typedef enum {
    STMT_EXPRESSION, STMT_VARIABLE_DECLARATION, STMT_CONSTANT_DECLARATION,
    STMT_ASSIGNMENT, STMT_BLOCK, STMT_IF, STMT_WHILE, STMT_FOR, STMT_FOR_EACH,
    STMT_BREAK, STMT_CONTINUE, STMT_FUNCTION, STMT_RETURN,
    STMT_INDEX_ASSIGNMENT, STMT_IMPORT
} StmtType;
typedef struct Stmt Stmt;
typedef struct { Stmt **data; size_t count; size_t capacity; } StmtArray;
struct Stmt {
    StmtType type;
    SourceSpan span;
    bool exported;
    union {
        struct { Expr *expression; } expression;
        struct { char *name; size_t name_length; SourceSpan name_span; Expr *initializer; } declaration;
        struct { char *name; size_t name_length; SourceSpan name_span; Expr *value; } assignment;
        struct { StmtArray statements; } block;
        struct { Expr *condition; Stmt *then_branch; Stmt *else_branch; } if_statement;
        struct { Expr *condition; Stmt *body; } while_statement;
        struct {
            char *iterator_name;
            size_t iterator_length;
            SourceSpan iterator_span;
            Expr *start;
            Expr *end;
            Stmt *body;
        } for_statement;
        struct {
            char *iterator_name;
            size_t iterator_length;
            SourceSpan iterator_span;
            Expr *iterable;
            Stmt *body;
        } for_each_statement;
        struct {
            char *name; size_t name_length; SourceSpan name_span;
            char **parameters; size_t *parameter_lengths; SourceSpan *parameter_spans;
            size_t parameter_count; Stmt *body;
        } function;
        struct { Expr *value; } return_statement;
        struct { Expr *target; Expr *index; Expr *value; } index_assignment;
        struct { char *path; size_t path_length; char *binding; size_t binding_length; SourceSpan path_span; } import;
    } as;
};
typedef struct { StmtArray statements; } Program;

void program_init(Program *program);
Program *program_new(void);
bool program_add_statement(Program *program, Stmt *statement);
void program_free(Program *program);
Stmt *stmt_new_expression(Expr *expression);
Stmt *stmt_new_declaration(bool mutable, const char *name, size_t name_length,
                           SourceSpan name_span, Expr *initializer);
Stmt *stmt_new_assignment(const char *name, size_t name_length,
                          SourceSpan name_span, Expr *value);
Stmt *stmt_new_block(StmtArray statements, SourceSpan span);
Stmt *stmt_new_if(Expr *condition, Stmt *then_branch, Stmt *else_branch, SourceSpan span);
Stmt *stmt_new_while(Expr *condition, Stmt *body, SourceSpan span);
Stmt *stmt_new_for(const char *name, size_t name_length, SourceSpan name_span,
                   Expr *start, Expr *end, Stmt *body, SourceSpan span);
Stmt *stmt_new_for_each(const char *name, size_t name_length, SourceSpan name_span,
                        Expr *iterable, Stmt *body, SourceSpan span);
Stmt *stmt_new_loop_control(bool is_break, SourceSpan span);
Stmt *stmt_new_function(const char *name, size_t name_length, SourceSpan name_span,
                        char **parameters, size_t *parameter_lengths,
                        SourceSpan *parameter_spans, size_t parameter_count,
                        Stmt *body, SourceSpan span);
Stmt *stmt_new_return(Expr *value, SourceSpan span);
Stmt *stmt_new_index_assignment(Expr *target, Expr *index, Expr *value, SourceSpan span);
Stmt *stmt_new_import(const char *path, size_t path_length, SourceSpan path_span,
                      SourceSpan span);
bool stmt_array_add(StmtArray *array, Stmt *statement);
void stmt_array_free(StmtArray *array);
void stmt_free(Stmt *statement);
#endif

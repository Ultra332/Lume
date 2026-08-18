#include "ast.h"
#include <string.h>
#include "memory.h"
static Expr *expr_allocate(ExprType type, SourceSpan span) {
    Expr *expression = memory_allocate(sizeof(*expression));
    if (expression != NULL) { expression->type = type; expression->span = span; }
    return expression;
}
Expr *expr_new_literal(Value value, SourceSpan span) {
    Expr *expression = expr_allocate(EXPR_LITERAL, span);
    if (expression != NULL) expression->as.literal = value;
    return expression;
}
Expr *expr_new_identifier(const char *name, size_t length, SourceSpan span) {
    Expr *expression = expr_allocate(EXPR_IDENTIFIER, span);
    char *copy;
    if (expression == NULL) return NULL;
    copy = memory_copy_string(name, length);
    if (copy == NULL) { memory_free(expression); return NULL; }
    expression->as.identifier.name = copy; expression->as.identifier.length = length;
    return expression;
}
Expr *expr_new_unary(UnaryOperator operator_type, SourceSpan operator_span, Expr *operand) {
    SourceSpan span = operator_span;
    Expr *expression;
    if (operand != NULL) span.end = operand->span.end;
    expression = expr_allocate(EXPR_UNARY, span);
    if (expression != NULL) {
        expression->as.unary.operator_type = operator_type;
        expression->as.unary.operator_span = operator_span;
        expression->as.unary.operand = operand;
    }
    return expression;
}
Expr *expr_new_binary(Expr *left, BinaryOperator operator_type, SourceSpan operator_span, Expr *right) {
    SourceSpan span = operator_span;
    Expr *expression;
    if (left != NULL) span.start = left->span.start;
    if (right != NULL) span.end = right->span.end;
    expression = expr_allocate(EXPR_BINARY, span);
    if (expression != NULL) {
        expression->as.binary.left = left; expression->as.binary.operator_type = operator_type;
        expression->as.binary.operator_span = operator_span; expression->as.binary.right = right;
    }
    return expression;
}
Expr *expr_new_grouping(Expr *inner, SourceSpan span) {
    Expr *expression = expr_allocate(EXPR_GROUPING, span);
    if (expression != NULL) expression->as.grouping.expression = inner;
    return expression;
}
Expr *expr_new_call(Expr *callee, Expr **arguments, size_t argument_count, SourceSpan span) {
    Expr *expression = expr_allocate(EXPR_CALL, span);
    if (expression != NULL) {
        expression->as.call.callee = callee;
        expression->as.call.arguments = arguments;
        expression->as.call.argument_count = argument_count;
    }
    return expression;
}
Expr *expr_new_list(Expr **elements, size_t count, SourceSpan span) { Expr *e=expr_allocate(EXPR_LIST,span);if(e!=NULL){e->as.list.elements=elements;e->as.list.count=count;}return e; }
Expr *expr_new_index(Expr *target,Expr *index,SourceSpan span){Expr *e=expr_allocate(EXPR_INDEX,span);if(e!=NULL){e->as.index.target=target;e->as.index.index=index;}return e;}
Expr *expr_new_member(Expr *target,const char *name,size_t length,SourceSpan name_span,SourceSpan span){Expr *e=expr_allocate(EXPR_MEMBER,span);char *copy;if(e==NULL)return NULL;copy=memory_copy_string(name,length);if(copy==NULL){memory_free(e);return NULL;}e->as.member.target=target;e->as.member.name=copy;e->as.member.name_length=length;e->as.member.name_span=name_span;return e;}
void expr_free(Expr *expression) {
    if (expression == NULL) return;
    switch (expression->type) {
        case EXPR_LITERAL: value_free(&expression->as.literal); break;
        case EXPR_IDENTIFIER: memory_free(expression->as.identifier.name); break;
        case EXPR_UNARY: expr_free(expression->as.unary.operand); break;
        case EXPR_BINARY:
            expr_free(expression->as.binary.left); expr_free(expression->as.binary.right); break;
        case EXPR_GROUPING: expr_free(expression->as.grouping.expression); break;
        case EXPR_CALL: {
            size_t index;
            expr_free(expression->as.call.callee);
            for (index = 0U; index < expression->as.call.argument_count; index++)
                expr_free(expression->as.call.arguments[index]);
            memory_free(expression->as.call.arguments);
            break;
        }
        case EXPR_LIST: { size_t index; for(index=0U;index<expression->as.list.count;index++)expr_free(expression->as.list.elements[index]);memory_free(expression->as.list.elements);break; }
        case EXPR_INDEX: expr_free(expression->as.index.target);expr_free(expression->as.index.index);break;
        case EXPR_MEMBER: expr_free(expression->as.member.target);memory_free(expression->as.member.name);break;
    }
    memory_free(expression);
}

static void stmt_array_init(StmtArray *array) {
    array->data = NULL; array->count = 0U; array->capacity = 0U;
}
bool stmt_array_add(StmtArray *array, Stmt *statement) {
    Stmt **grown;
    size_t capacity;
    if (array == NULL || statement == NULL) return false;
    if (array->count == array->capacity) {
        if (array->count == SIZE_MAX ||
            !memory_grow_capacity(array->capacity, array->count + 1U, &capacity)) return false;
        grown = memory_reallocate_array(array->data, capacity, sizeof(*grown));
        if (grown == NULL) return false;
        array->data = grown; array->capacity = capacity;
    }
    array->data[array->count++] = statement;
    return true;
}
void stmt_array_free(StmtArray *array) {
    size_t index;
    if (array == NULL) return;
    for (index = 0U; index < array->count; index++) stmt_free(array->data[index]);
    memory_free(array->data);
    stmt_array_init(array);
}
void program_init(Program *program) {
    if (program != NULL) stmt_array_init(&program->statements);
}
Program *program_new(void) {
    Program *program = memory_allocate(sizeof(*program));
    if (program != NULL) program_init(program);
    return program;
}
bool program_add_statement(Program *program, Stmt *statement) {
    return program != NULL && stmt_array_add(&program->statements, statement);
}
void program_free(Program *program) {
    if (program != NULL) { stmt_array_free(&program->statements); memory_free(program); }
}
static Stmt *stmt_allocate(StmtType type, SourceSpan span) {
    Stmt *statement = memory_allocate(sizeof(*statement));
    if (statement != NULL) { statement->type = type; statement->span = span; statement->exported=false; }
    return statement;
}
Stmt *stmt_new_expression(Expr *expression) {
    Stmt *statement;
    if (expression == NULL) return NULL;
    statement = stmt_allocate(STMT_EXPRESSION, expression->span);
    if (statement != NULL) statement->as.expression.expression = expression;
    return statement;
}
Stmt *stmt_new_declaration(bool mutable, const char *name, size_t name_length,
                           SourceSpan name_span, Expr *initializer) {
    StmtType type = mutable ? STMT_VARIABLE_DECLARATION : STMT_CONSTANT_DECLARATION;
    Stmt *statement;
    char *copy;
    SourceSpan span = name_span;
    if (initializer == NULL) return NULL;
    span.end = initializer->span.end;
    statement = stmt_allocate(type, span);
    if (statement == NULL) return NULL;
    copy = memory_copy_string(name, name_length);
    if (copy == NULL) { memory_free(statement); return NULL; }
    statement->as.declaration.name = copy;
    statement->as.declaration.name_length = name_length;
    statement->as.declaration.name_span = name_span;
    statement->as.declaration.initializer = initializer;
    return statement;
}
Stmt *stmt_new_assignment(const char *name, size_t name_length,
                          SourceSpan name_span, Expr *value) {
    Stmt *statement;
    char *copy;
    SourceSpan span = name_span;
    if (value == NULL) return NULL;
    span.end = value->span.end;
    statement = stmt_allocate(STMT_ASSIGNMENT, span);
    if (statement == NULL) return NULL;
    copy = memory_copy_string(name, name_length);
    if (copy == NULL) { memory_free(statement); return NULL; }
    statement->as.assignment.name = copy;
    statement->as.assignment.name_length = name_length;
    statement->as.assignment.name_span = name_span;
    statement->as.assignment.value = value;
    return statement;
}
Stmt *stmt_new_block(StmtArray statements, SourceSpan span) {
    Stmt *statement = stmt_allocate(STMT_BLOCK, span);
    if (statement != NULL) statement->as.block.statements = statements;
    return statement;
}
Stmt *stmt_new_if(Expr *condition, Stmt *then_branch, Stmt *else_branch, SourceSpan span) {
    Stmt *statement;
    if (condition == NULL || then_branch == NULL) return NULL;
    statement = stmt_allocate(STMT_IF, span);
    if (statement != NULL) {
        statement->as.if_statement.condition = condition;
        statement->as.if_statement.then_branch = then_branch;
        statement->as.if_statement.else_branch = else_branch;
    }
    return statement;
}
Stmt *stmt_new_while(Expr *condition, Stmt *body, SourceSpan span) {
    Stmt *statement;
    if (condition == NULL || body == NULL) return NULL;
    statement = stmt_allocate(STMT_WHILE, span);
    if (statement != NULL) {
        statement->as.while_statement.condition = condition;
        statement->as.while_statement.body = body;
    }
    return statement;
}
Stmt *stmt_new_for(const char *name, size_t name_length, SourceSpan name_span,
                   Expr *start, Expr *end, Stmt *body, SourceSpan span) {
    Stmt *statement;
    char *copy;
    if (name == NULL || start == NULL || end == NULL || body == NULL) return NULL;
    statement = stmt_allocate(STMT_FOR, span);
    if (statement == NULL) return NULL;
    copy = memory_copy_string(name, name_length);
    if (copy == NULL) { memory_free(statement); return NULL; }
    statement->as.for_statement.iterator_name = copy;
    statement->as.for_statement.iterator_length = name_length;
    statement->as.for_statement.iterator_span = name_span;
    statement->as.for_statement.start = start;
    statement->as.for_statement.end = end;
    statement->as.for_statement.body = body;
    return statement;
}
Stmt *stmt_new_function(const char *name, size_t name_length, SourceSpan name_span,
                        char **parameters, size_t *parameter_lengths,
                        SourceSpan *parameter_spans, size_t parameter_count,
                        Stmt *body, SourceSpan span) {
    Stmt *statement = stmt_allocate(STMT_FUNCTION, span);
    char *copy;
    if (statement == NULL) return NULL;
    copy = memory_copy_string(name, name_length);
    if (copy == NULL) { memory_free(statement); return NULL; }
    statement->as.function.name = copy;
    statement->as.function.name_length = name_length;
    statement->as.function.name_span = name_span;
    statement->as.function.parameters = parameters;
    statement->as.function.parameter_lengths = parameter_lengths;
    statement->as.function.parameter_spans = parameter_spans;
    statement->as.function.parameter_count = parameter_count;
    statement->as.function.body = body;
    return statement;
}
Stmt *stmt_new_return(Expr *value, SourceSpan span) {
    Stmt *statement = stmt_allocate(STMT_RETURN, span);
    if (statement != NULL) statement->as.return_statement.value = value;
    return statement;
}
Stmt *stmt_new_index_assignment(Expr *target,Expr *index,Expr *value,SourceSpan span){Stmt *s=stmt_allocate(STMT_INDEX_ASSIGNMENT,span);if(s!=NULL){s->as.index_assignment.target=target;s->as.index_assignment.index=index;s->as.index_assignment.value=value;}return s;}
Stmt *stmt_new_import(const char *path,size_t path_length,SourceSpan path_span,SourceSpan span){Stmt *s=stmt_allocate(STMT_IMPORT,span);size_t start=0U,i;char *binding,*copy;if(s==NULL)return NULL;copy=memory_copy_string(path,path_length);if(copy==NULL){memory_free(s);return NULL;}for(i=0U;i<path_length;i++)if(path[i]=='/'||path[i]=='\\')start=i+1U;{size_t length=path_length-start;if(length>=5U&&memcmp(path+path_length-5U,".lume",5U)==0)length-=5U;binding=memory_copy_string(path+start,length);if(binding==NULL){memory_free(copy);memory_free(s);return NULL;}s->as.import.path=copy;s->as.import.path_length=path_length;s->as.import.binding=binding;s->as.import.binding_length=length;s->as.import.path_span=path_span;}return s;}
void stmt_free(Stmt *statement) {
    if (statement == NULL) return;
    switch (statement->type) {
        case STMT_EXPRESSION: expr_free(statement->as.expression.expression); break;
        case STMT_VARIABLE_DECLARATION:
        case STMT_CONSTANT_DECLARATION:
            memory_free(statement->as.declaration.name);
            expr_free(statement->as.declaration.initializer);
            break;
        case STMT_ASSIGNMENT:
            memory_free(statement->as.assignment.name);
            expr_free(statement->as.assignment.value);
            break;
        case STMT_BLOCK: stmt_array_free(&statement->as.block.statements); break;
        case STMT_IF:
            expr_free(statement->as.if_statement.condition);
            stmt_free(statement->as.if_statement.then_branch);
            stmt_free(statement->as.if_statement.else_branch);
            break;
        case STMT_WHILE:
            expr_free(statement->as.while_statement.condition);
            stmt_free(statement->as.while_statement.body);
            break;
        case STMT_FOR:
            memory_free(statement->as.for_statement.iterator_name);
            expr_free(statement->as.for_statement.start);
            expr_free(statement->as.for_statement.end);
            stmt_free(statement->as.for_statement.body);
            break;
        case STMT_FUNCTION: {
            size_t index;
            memory_free(statement->as.function.name);
            for (index = 0U; index < statement->as.function.parameter_count; index++)
                memory_free(statement->as.function.parameters[index]);
            memory_free(statement->as.function.parameters);
            memory_free(statement->as.function.parameter_lengths);
            memory_free(statement->as.function.parameter_spans);
            stmt_free(statement->as.function.body);
            break;
        }
        case STMT_RETURN: expr_free(statement->as.return_statement.value); break;
        case STMT_INDEX_ASSIGNMENT: expr_free(statement->as.index_assignment.target);expr_free(statement->as.index_assignment.index);expr_free(statement->as.index_assignment.value);break;
        case STMT_IMPORT: memory_free(statement->as.import.path);memory_free(statement->as.import.binding);break;
    }
    memory_free(statement);
}

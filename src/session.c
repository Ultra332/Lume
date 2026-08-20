#include "session.h"
#include <string.h>
#include "memory.h"
static bool statement_has_function(const Stmt *statement) {
    size_t index;
    if (statement == NULL) return false;
    if (statement->type == STMT_FUNCTION) return true;
    if (statement->type == STMT_BLOCK) {
        for (index = 0U; index < statement->as.block.statements.count; index++)
            if (statement_has_function(statement->as.block.statements.data[index])) return true;
    }
    if (statement->type == STMT_IF)
        return statement_has_function(statement->as.if_statement.then_branch) ||
            statement_has_function(statement->as.if_statement.else_branch);
    if (statement->type == STMT_WHILE) return statement_has_function(statement->as.while_statement.body);
    if (statement->type == STMT_FOR) return statement_has_function(statement->as.for_statement.body);
    if (statement->type == STMT_FOR_EACH) return statement_has_function(statement->as.for_each_statement.body);
    return false;
}
static bool program_has_function(const Program *program) {
    size_t index;
    for (index = 0U; index < program->statements.count; index++)
        if (statement_has_function(program->statements.data[index])) return true;
    return false;
}
void session_init(LumeSession *session, RuntimeIO io) {
    environment_init(&session->environment, NULL); session->io = io;
    module_registry_init(&session->modules,&session->io,NULL);
    session->programs = NULL; session->sources = NULL; session->retained_count = 0U; session->retained_capacity = 0U;
}
void session_free(LumeSession *session) {
    size_t index;
    environment_free(&session->environment);
    module_registry_free(&session->modules);
    for (index = 0U; index < session->retained_count; index++) {
        program_free(session->programs[index]); source_free(session->sources[index]); memory_free(session->sources[index]);
    }
    memory_free(session->programs); memory_free(session->sources);
}
InputStatus session_classify(const char *text, size_t length) {
    Source source; TokenArray tokens; ErrorList errors; bool ok; size_t index; int braces = 0, parens = 0, brackets = 0;
    source_init(&source); token_array_init(&tokens); error_list_init(&errors);
    ok = source_from_bytes(&source, "<entrada>", text, length);
    if (ok) ok = lexer_scan(&source, &tokens, &errors);
    if (!ok) { error_list_free(&errors); token_array_free(&tokens); source_free(&source); return INPUT_INVALID; }
    for (index = 0U; index < tokens.count; index++) {
        TokenType type = tokens.data[index].type;
        if (type == TOKEN_LEFT_BRACE) braces++;
        else if (type == TOKEN_RIGHT_BRACE) braces--;
        else if (type == TOKEN_LEFT_PAREN) parens++;
        else if (type == TOKEN_RIGHT_PAREN) parens--;
        else if (type == TOKEN_LEFT_BRACKET) brackets++;
        else if (type == TOKEN_RIGHT_BRACKET) brackets--;
        if (braces < 0 || parens < 0 || brackets < 0) break;
    }
    if (braces == 0 && parens == 0 && tokens.count > 1U) {
        TokenType last = tokens.data[tokens.count - 2U].type;
        if (last == TOKEN_PLUS || last == TOKEN_MINUS || last == TOKEN_STAR || last == TOKEN_SLASH ||
            last == TOKEN_PERCENT || last == TOKEN_EQUAL || last == TOKEN_EQUAL_EQUAL ||
            last == TOKEN_BANG_EQUAL || last == TOKEN_LESS || last == TOKEN_LESS_EQUAL ||
            last == TOKEN_GREATER || last == TOKEN_GREATER_EQUAL || last == TOKEN_COMMA ||
            last == TOKEN_KW_E || last == TOKEN_KW_OU || last == TOKEN_KW_NAO ||
            last == TOKEN_KW_VARIAVEL || last == TOKEN_KW_CONSTANTE || last == TOKEN_KW_FUNCAO ||
            last == TOKEN_KW_SE || last == TOKEN_KW_SENAO || last == TOKEN_KW_ENQUANTO ||
            last == TOKEN_KW_PARA || last == TOKEN_KW_DE || last == TOKEN_KW_ATE)
            parens = 1;
    }
    error_list_free(&errors); token_array_free(&tokens); source_free(&source);
    return braces > 0 || parens > 0 || brackets > 0 ? INPUT_INCOMPLETE : INPUT_COMPLETE;
}
static bool retain(LumeSession *session, Program *program, Source *source) {
    Program **programs; Source **sources; size_t capacity;
    if (session->retained_count == session->retained_capacity) {
        if (!memory_grow_capacity(session->retained_capacity, session->retained_count + 1U, &capacity)) return false;
        programs = memory_reallocate_array(session->programs, capacity, sizeof(*programs)); if (programs == NULL) return false;
        session->programs = programs;
        sources = memory_reallocate_array(session->sources, capacity, sizeof(*sources)); if (sources == NULL) return false;
        session->sources = sources; session->retained_capacity = capacity;
    }
    session->programs[session->retained_count] = program; session->sources[session->retained_count] = source;
    session->retained_count++; return true;
}
static bool session_execute_internal(LumeSession *session, const char *name, const char *text,
                     size_t length, bool print_expression, bool suppress_null_call,
                     Source **error_source, ErrorList *errors) {
    Source *source = memory_allocate(sizeof(*source)); TokenArray tokens; Program *program = NULL; bool ok;
    Value result = value_null();
    if (source == NULL) return false;
    source_init(source); token_array_init(&tokens); *error_source = source;
    ok = source_from_bytes(source, name, text, length);
    if (ok) ok = lexer_scan(source, &tokens, errors);
    if (ok) ok = parser_parse_program(&tokens, &program, errors);
    if (ok && !environment_has_current(&session->environment, "escreva", 7U)) {
        Program empty; program_init(&empty);
        ok = interpreter_execute_program_with_modules(&empty,&session->environment,&session->io,NULL,&session->modules,NULL,errors);
    }
    if (ok && print_expression && program->statements.count == 1U &&
        program->statements.data[0]->type == STMT_EXPRESSION) {
        ok = interpreter_evaluate_expression_with_io(program->statements.data[0]->as.expression.expression,
            &session->environment, &session->io, &result, errors);
        if (ok && (!suppress_null_call || result.type != VALUE_NULL ||
                   program->statements.data[0]->as.expression.expression->type != EXPR_CALL)) {
            value_print(session->io.output, &result); fputc('\n', session->io.output);
        }
        value_free(&result);
    } else if (ok) { LumeModule current; memset(&current,0,sizeof(current)); current.path=(char *)name;
        ok=interpreter_execute_program_with_modules(program,&session->environment,&session->io,NULL,&session->modules,&current,errors); }
    token_array_free(&tokens);
    if (ok && program_has_function(program)) {
        if (!retain(session, program, source)) ok = false;
    } else if (ok) {
        program_free(program); source_free(source); memory_free(source); *error_source = NULL;
    }
    if (!ok && program != NULL) program_free(program);
    return ok;
}
bool session_execute(LumeSession *session, const char *name, const char *text, size_t length,
                     bool print_expression, Source **error_source, ErrorList *errors) {
    return session_execute_internal(session,name,text,length,print_expression,false,error_source,errors);
}
bool session_execute_repl(LumeSession *session, const char *name, const char *text, size_t length,
                          Source **error_source, ErrorList *errors) {
    return session_execute_internal(session,name,text,length,true,true,error_source,errors);
}

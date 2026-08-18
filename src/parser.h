#ifndef LUME_PARSER_H
#define LUME_PARSER_H
#include "ast.h"
#include "token.h"

/* Tokens and their Source must outlive the returned AST. Caller owns out_expr. */
bool parser_parse_expression(const TokenArray *tokens, Expr **out_expr, ErrorList *errors);
bool parser_parse_program(const TokenArray *tokens, Program **out_program, ErrorList *errors);

#endif

#ifndef LUME_LEXER_H
#define LUME_LEXER_H
#include "source.h"
#include "token.h"

/* Outputs must be initialized and empty. On lexical error scanning stops. */
bool lexer_scan(const Source *source, TokenArray *tokens, ErrorList *errors);

#endif

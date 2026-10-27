#ifndef FAILSIM_LEXER_H
#define FAILSIM_LEXER_H
#include <stddef.h>
#include "token.h"

typedef struct {
    const char *source;
    size_t pos;
    int line;
    int column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next(Lexer *lexer);
void token_free(Token *token);

#endif

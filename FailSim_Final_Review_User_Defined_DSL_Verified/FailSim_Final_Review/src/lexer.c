#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *word; TokenType type; } Keyword;

static const Keyword keywords[] = {
    {"SYSTEM", TOKEN_SYSTEM}, {"COMPONENT", TOKEN_COMPONENT},
    {"TYPE", TOKEN_TYPE}, {"DEPENDS", TOKEN_DEPENDS},
    {"OPTIONAL", TOKEN_OPTIONAL}, {"ON", TOKEN_ON},
    {"CONNECTS", TOKEN_CONNECTS}, {"TO", TOKEN_TO},
    {"STATE", TOKEN_STATE}, {"RECOVERY", TOKEN_RECOVERY},
    {"USE", TOKEN_USE}, {"REDIRECT", TOKEN_REDIRECT},
    {"SCENARIO", TOKEN_SCENARIO}, {"FAIL", TOKEN_FAIL},
    {"END", TOKEN_END}, {"GROUP", TOKEN_GROUP},
    {"CRITICALITY", TOKEN_CRITICALITY},
    {"CLIENT", TOKEN_CLIENT}, {"CDN", TOKEN_CDN},
    {"GATEWAY", TOKEN_GATEWAY}, {"LOAD_BALANCER", TOKEN_LOAD_BALANCER},
    {"AUTH", TOKEN_AUTH}, {"SERVICE", TOKEN_SERVICE},
    {"DATABASE", TOKEN_DATABASE}, {"CACHE", TOKEN_CACHE},
    {"QUEUE", TOKEN_QUEUE}, {"WORKER", TOKEN_WORKER},
    {"STORAGE", TOKEN_STORAGE}, {"EXTERNAL_API", TOKEN_EXTERNAL_API},
    {"MONITOR", TOKEN_MONITOR}, {"BACKUP", TOKEN_BACKUP},
    {"RUNNING", TOKEN_RUNNING}, {"STOPPED", TOKEN_STOPPED},
    {"STANDBY", TOKEN_STANDBY}, {"FAILED", TOKEN_FAILED},
    {"LOW", TOKEN_LOW}, {"MEDIUM", TOKEN_MEDIUM},
    {"HIGH", TOKEN_HIGH}, {"CRITICAL", TOKEN_CRITICAL}
};

static char *copy_range(const char *s, size_t a, size_t b) {
    size_t n = b - a;
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s + a, n);
    out[n] = '\0';
    return out;
}

static Token make_token(Lexer *l, TokenType type, size_t start, int line, int col) {
    Token t = {type, copy_range(l->source, start, l->pos), line, col};
    return t;
}

static TokenType keyword_type(const char *word) {
    size_t n = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < n; ++i)
        if (strcmp(word, keywords[i].word) == 0) return keywords[i].type;
    return TOKEN_IDENTIFIER;
}

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
}

Token lexer_next(Lexer *l) {
    while (1) {
        char c = l->source[l->pos];
        if (c == '\0') {
            Token eof = {TOKEN_EOF, copy_range(l->source, l->pos, l->pos), l->line, l->column};
            return eof;
        }
        if (c == ' ' || c == '\t' || c == '\r') { l->pos++; l->column++; continue; }
        if (c == '\n') { l->pos++; l->line++; l->column = 1; continue; }
        if (c == '#') {
            while (l->source[l->pos] && l->source[l->pos] != '\n') { l->pos++; l->column++; }
            continue;
        }
        break;
    }

    size_t start = l->pos;
    int line = l->line, col = l->column;
    while (isalnum((unsigned char)l->source[l->pos]) || l->source[l->pos] == '_') {
        l->pos++; l->column++;
    }

    if (l->pos == start) {
        l->pos++; l->column++;
        return make_token(l, TOKEN_IDENTIFIER, start, line, col);
    }

    Token t = make_token(l, TOKEN_IDENTIFIER, start, line, col);
    if (!t.lexeme) return t;
    t.type = keyword_type(t.lexeme);
    return t;
}

void token_free(Token *token) {
    free(token->lexeme);
    token->lexeme = NULL;
}

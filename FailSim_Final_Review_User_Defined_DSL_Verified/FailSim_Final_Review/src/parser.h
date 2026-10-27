#ifndef FAILSIM_PARSER_H
#define FAILSIM_PARSER_H
#include "ast.h"
#include "lexer.h"
int parse_program(const char *source, Program *program, char *error, int error_size);
#endif

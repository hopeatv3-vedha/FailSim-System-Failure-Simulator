#ifndef FAILSIM_SEMANTIC_H
#define FAILSIM_SEMANTIC_H
#include "ast.h"
int semantic_check(const Program *p, char *error, int error_size);
#endif

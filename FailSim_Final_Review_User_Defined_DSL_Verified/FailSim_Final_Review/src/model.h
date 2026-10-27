#ifndef FAILSIM_MODEL_H
#define FAILSIM_MODEL_H
#include "ast.h"

typedef enum { R_HEALTHY, R_DEGRADED, R_CRITICAL, R_FAILED } RuntimeStatus;

typedef struct {
    Component c;
    RuntimeStatus status;
    int failed, direct_impact, indirect_impact, optional_impact, recovered;
    char active_redirect[FS_NAME];
} RuntimeComponent;

typedef struct {
    Program program;
    RuntimeComponent nodes[FS_MAX_COMPONENTS];
} SystemModel;

void model_build(const Program *p, SystemModel *m);
void model_print(const SystemModel *m);

#endif

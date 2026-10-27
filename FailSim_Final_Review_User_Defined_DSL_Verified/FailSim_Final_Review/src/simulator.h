#ifndef FAILSIM_SIMULATOR_H
#define FAILSIM_SIMULATOR_H
#include "model.h"

typedef struct {
    int failed_count, direct_count, indirect_count, optional_count, affected_count;
    int critical_failed, affected_groups;
    double impact_percent;
    RuntimeStatus system_status;
} AnalysisResult;

int simulate_scenario(SystemModel *m, const Scenario *s, AnalysisResult *result);
void print_analysis(const SystemModel *m, const Scenario *s, const AnalysisResult *r);

#endif

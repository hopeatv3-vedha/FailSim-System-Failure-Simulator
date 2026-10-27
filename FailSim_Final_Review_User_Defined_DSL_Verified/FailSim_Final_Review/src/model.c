#include "model.h"
#include <stdio.h>
#include <string.h>

static const char *type_name(ComponentType t) {
    const char *a[]={"CLIENT","CDN","GATEWAY","LOAD_BALANCER","AUTH","SERVICE","DATABASE","CACHE","QUEUE","WORKER","STORAGE","EXTERNAL_API","MONITOR","BACKUP","UNKNOWN"};
    return a[t];
}
static const char *state_name(State s) { const char *a[]={"RUNNING","STOPPED","STANDBY","FAILED"}; return a[s]; }
static const char *crit_name(Criticality c) { const char *a[]={"LOW","MEDIUM","HIGH","CRITICAL"}; return a[c]; }

void model_build(const Program *p, SystemModel *m) {
    m->program=*p;
    for(int i=0;i<p->component_count;i++) {
        m->nodes[i].c=p->components[i];
        m->nodes[i].status=R_HEALTHY;
        m->nodes[i].failed=(p->components[i].state==STATE_FAILED);
        if(m->nodes[i].failed) m->nodes[i].status=R_FAILED;
        m->nodes[i].direct_impact=m->nodes[i].indirect_impact=0;
        m->nodes[i].optional_impact=m->nodes[i].recovered=0;
        m->nodes[i].active_redirect[0]='\0';
    }
}

void model_print(const SystemModel *m) {
    printf("\n=== COMPILED SYSTEM MODEL ===\n");
    printf("System: %s\nComponents: %d\nMandatory dependencies: %d\nOptional dependencies: %d\nConnections: %d\nGroups: %d\n",
        m->program.system_name,m->program.component_count,m->program.depends_count,
        m->program.optional_count,m->program.connection_count,m->program.group_count);
    for(int i=0;i<m->program.component_count;i++) {
        const Component *c=&m->nodes[i].c;
        printf("  %-20s %-15s %-9s %-8s",c->name,type_name(c->type),state_name(c->state),crit_name(c->criticality));
        if(c->has_group) printf(" group=%s",c->group);
        putchar('\n');
    }
    printf("\nMandatory dependency graph:\n");
    for(int i=0;i<m->program.depends_count;i++) printf("  %s -> %s\n",m->program.depends[i].source,m->program.depends[i].target);
    printf("Optional dependency graph:\n");
    for(int i=0;i<m->program.optional_count;i++) printf("  %s ~> %s\n",m->program.optional[i].source,m->program.optional[i].target);
}

#include "recovery.h"
#include <stdio.h>
#include <string.h>

static int idx(const SystemModel *m,const char *n){for(int i=0;i<m->program.component_count;i++)if(!strcmp(m->nodes[i].c.name,n))return i;return -1;}
static Recovery *policy(SystemModel *m,const char *failed){
    for(int i=0;i<m->program.recovery_count;i++)if(!strcmp(m->program.recoveries[i].failed,failed))return &m->program.recoveries[i];
    return NULL;
}
void apply_recovery(SystemModel *m,const Scenario *s){
    for(int f=0;f<s->failure_count;f++){
        Recovery *r=policy(m,s->failures[f]); if(!r)continue;
        if(r->has_backup){
            int b=idx(m,r->backup);
            if(b>=0){m->nodes[b].status=R_HEALTHY;m->nodes[b].c.state=STATE_RUNNING;}
        }
        for(int j=0;j<r->redirect_count;j++){
            int src=idx(m,r->redirects[j].source),dst=idx(m,r->redirects[j].target);
            if(src>=0&&dst>=0){
                char target_name[FS_NAME];
                snprintf(target_name, FS_NAME, "%s", m->nodes[dst].c.name);
                snprintf(m->nodes[src].active_redirect, FS_NAME, "%s", target_name);
                if(m->nodes[src].status!=R_FAILED){m->nodes[src].status=R_HEALTHY;m->nodes[src].recovered=1;}
            }
        }
    }
}
void print_recovery(const SystemModel *m,const Scenario *s){
    printf("\n=== FAILSIM RECOVERY REPORT ===\n");
    int recovered=0, backups=0, redirects=0;
    for(int f=0;f<s->failure_count;f++){
        Recovery *r=policy((SystemModel*)m,s->failures[f]);
        if(!r)continue;
        if(r->has_backup){printf("Backup activated: %s\n",r->backup);backups++;}
        for(int j=0;j<r->redirect_count;j++){printf("Redirect: %s -> %s\n",r->redirects[j].source,r->redirects[j].target);redirects++;}
    }
    for(int i=0;i<m->program.component_count;i++)if(m->nodes[i].recovered)recovered++;
    printf("Backup actions: %d\nRedirect actions: %d\nRecovered components: %d\n",backups,redirects,recovered);
    printf("\nPost-recovery state:\n");
    for(int i=0;i<m->program.component_count;i++){
        const RuntimeComponent *x=&m->nodes[i];
        printf("  %-20s %s",x->c.name,x->status==R_FAILED?"FAILED":(x->status==R_DEGRADED?"DEGRADED":"RUNNING"));
        if(x->active_redirect[0])printf(" -> %s",x->active_redirect);
        putchar('\n');
    }
}

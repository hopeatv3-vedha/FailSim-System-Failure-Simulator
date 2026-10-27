#include "simulator.h"
#include <stdio.h>
#include <string.h>

static int idx(const SystemModel *m,const char *n){for(int i=0;i<m->program.component_count;i++)if(!strcmp(m->nodes[i].c.name,n))return i;return -1;}
static int depends_on(const SystemModel *m,int source,int target){
    for(int i=0;i<m->program.depends_count;i++){
        int a=idx(m,m->program.depends[i].source),b=idx(m,m->program.depends[i].target);
        if(a==source&&b==target)return 1;
    } return 0;
}
static int optional_on(const SystemModel *m,int source,int target){
    for(int i=0;i<m->program.optional_count;i++){
        int a=idx(m,m->program.optional[i].source),b=idx(m,m->program.optional[i].target);
        if(a==source&&b==target)return 1;
    } return 0;
}
int simulate_scenario(SystemModel *m,const Scenario *s,AnalysisResult *r){
    memset(r,0,sizeof(*r));
    int n=m->program.component_count;
    int dist[FS_MAX_COMPONENTS]; for(int i=0;i<n;i++)dist[i]=-1;
    int q[FS_MAX_COMPONENTS],head=0,tail=0;
    /* Components declared STATE FAILED are initial failure roots too. */
    for(int i=0;i<n;i++) if(m->nodes[i].failed){dist[i]=0;m->nodes[i].status=R_FAILED;q[tail++]=i;}
    for(int f=0;f<s->failure_count;f++){
        int x=idx(m,s->failures[f]); if(x<0)continue;
        if(dist[x]>=0)continue;
        m->nodes[x].failed=1;m->nodes[x].status=R_FAILED;dist[x]=0;q[tail++]=x;
    }
    while(head<tail){
        int failed=q[head++];
        for(int j=0;j<n;j++){
            if(dist[j]!=-1) continue;
            if(depends_on(m,j,failed)){
                dist[j]=dist[failed]+1;
                m->nodes[j].status=R_CRITICAL;
                if(dist[j]==1)m->nodes[j].direct_impact=1;else m->nodes[j].indirect_impact=1;
                q[tail++]=j;
            }
        }
    }
    for(int i=0;i<n;i++){
        if(m->nodes[i].failed)continue;
        for(int f=0;f<s->failure_count;f++){
            int x=idx(m,s->failures[f]);
            if(optional_on(m,i,x)){m->nodes[i].optional_impact=1;if(m->nodes[i].status==R_HEALTHY)m->nodes[i].status=R_DEGRADED;}
        }
    }
    int groups[FS_MAX_GROUPS]={0};
    for(int i=0;i<n;i++){
        if(m->nodes[i].failed)r->failed_count++;
        if(m->nodes[i].direct_impact)r->direct_count++;
        if(m->nodes[i].indirect_impact)r->indirect_count++;
        if(m->nodes[i].optional_impact)r->optional_count++;
        /* Impact is a unique component count: overlapping dependency types must not double-count a node. */
        if(m->nodes[i].failed || m->nodes[i].direct_impact ||
           m->nodes[i].indirect_impact || m->nodes[i].optional_impact) r->affected_count++;
        if(m->nodes[i].failed && m->nodes[i].c.criticality==CRIT_CRITICAL)r->critical_failed=1;
        if(m->nodes[i].status!=R_HEALTHY && m->nodes[i].c.has_group){
            for(int g=0;g<m->program.group_count;g++)if(!strcmp(m->program.groups[g],m->nodes[i].c.group))groups[g]=1;
        }
    }
    for(int g=0;g<m->program.group_count;g++)r->affected_groups+=groups[g];
    int affected=r->affected_count;
    r->impact_percent=n?((double)affected*100.0/n):0;
    if(r->failed_count==n)r->system_status=R_FAILED;
    else if(r->critical_failed)r->system_status=R_CRITICAL;
    else if(affected>0)r->system_status=R_DEGRADED;
    else r->system_status=R_HEALTHY;
    return 1;
}
void print_analysis(const SystemModel *m,const Scenario *s,const AnalysisResult *r){
    const char *st[]={"HEALTHY","DEGRADED","CRITICAL","FAILED"};
    printf("\n=== FAILSIM FAILURE ANALYSIS ===\nScenario: %s\n",s->name);
    printf("Failed components   : %d\nDirect impact       : %d\nIndirect impact     : %d\nOptional degradation: %d\nAffected groups     : %d\nImpact              : %.1f%%\nSystem status       : %s\n",
        r->failed_count,r->direct_count,r->indirect_count,r->optional_count,r->affected_groups,r->impact_percent,st[r->system_status]);
    printf("\nComponent impact:\n");
    for(int i=0;i<m->program.component_count;i++){
        const RuntimeComponent *x=&m->nodes[i];
        if(x->status!=R_HEALTHY) printf("  %-20s %s%s%s\n",x->c.name,
            x->status==R_FAILED?"FAILED":(x->status==R_DEGRADED?"DEGRADED":"AFFECTED"),
            x->direct_impact?" [DIRECT]":"",x->indirect_impact?" [INDIRECT]":"");
    }
}

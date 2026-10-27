#include "semantic.h"
#include <stdio.h>
#include <string.h>

static int comp(const Program *p, const char *name) {
    for (int i=0;i<p->component_count;i++) if (!strcmp(p->components[i].name,name)) return i;
    return -1;
}
static int group(const Program *p, const char *name) {
    for (int i=0;i<p->group_count;i++) if (!strcmp(p->groups[i],name)) return i;
    return -1;
}
static int err(char *e,int n,const char *fmt,const char *a,const char *b) {
    if (e && n>0) snprintf(e,(size_t)n,fmt,a,b);
    return 0;
}
int semantic_check(const Program *p, char *error, int error_size) {
    for(int i=0;i<p->component_count;i++) {
        for(int j=i+1;j<p->component_count;j++)
            if(!strcmp(p->components[i].name,p->components[j].name))
                return err(error,error_size,"duplicate component: %s",p->components[i].name,"");
        if(p->components[i].has_group && group(p,p->components[i].group)<0)
            return err(error,error_size,"unknown group: %s",p->components[i].group,"");
    }
    for(int i=0;i<p->depends_count;i++) {
        if(comp(p,p->depends[i].source)<0 || comp(p,p->depends[i].target)<0)
            return err(error,error_size,"unknown dependency component: %s (source: %s)",p->depends[i].target,p->depends[i].source);
        if(!strcmp(p->depends[i].source,p->depends[i].target))
            return err(error,error_size,"self dependency: %s",p->depends[i].source,"");
    }
    for(int i=0;i<p->optional_count;i++) {
        if(comp(p,p->optional[i].source)<0 || comp(p,p->optional[i].target)<0)
            return err(error,error_size,"unknown optional dependency: %s",p->optional[i].source,"");
    }
    for(int i=0;i<p->connection_count;i++)
        if(comp(p,p->connects[i].source)<0 || comp(p,p->connects[i].target)<0)
            return err(error,error_size,"unknown connection component: %s",p->connects[i].source,"");
    for(int i=0;i<p->recovery_count;i++) {
        if(comp(p,p->recoveries[i].failed)<0) return err(error,error_size,"unknown recovery target: %s",p->recoveries[i].failed,"");
        if(p->recoveries[i].has_backup && comp(p,p->recoveries[i].backup)<0)
            return err(error,error_size,"unknown backup: %s",p->recoveries[i].backup,"");
        for(int j=0;j<p->recoveries[i].redirect_count;j++)
            if(comp(p,p->recoveries[i].redirects[j].source)<0 || comp(p,p->recoveries[i].redirects[j].target)<0)
                return err(error,error_size,"unknown redirect component: %s",p->recoveries[i].redirects[j].source,"");
    }
    for(int i=0;i<p->scenario_count;i++) {
        if(p->scenarios[i].failure_count==0) return err(error,error_size,"scenario has no failures: %s",p->scenarios[i].name,"");
        for(int j=0;j<p->scenarios[i].failure_count;j++)
            if(comp(p,p->scenarios[i].failures[j])<0)
                return err(error,error_size,"unknown failed component: %s",p->scenarios[i].failures[j],"");
    }
    return 1;
}

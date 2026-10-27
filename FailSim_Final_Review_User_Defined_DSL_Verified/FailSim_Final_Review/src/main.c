#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "model.h"
#include "simulator.h"
#include "recovery.h"

static char *read_file(const char *path){
    FILE *f=fopen(path,"rb"); if(!f)return NULL;
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    char *s=(char*)malloc((size_t)n+1); if(!s){fclose(f);return NULL;}
    size_t got=fread(s,1,(size_t)n,f); s[got]='\0'; fclose(f); return s;
}
static void js(const char *s){
    putchar('"');
    for(const unsigned char *p=(const unsigned char*)(s?s:"");*p;p++){
        switch(*p){case '"': fputs("\\\"",stdout); break; case '\\': fputs("\\\\",stdout); break;
        case '\n': fputs("\\n",stdout); break; case '\r': fputs("\\r",stdout); break; case '\t': fputs("\\t",stdout); break;
        default: if(*p<32) printf("\\u%04x",*p); else putchar(*p);}
    }
    putchar('"');
}
static const char *type_name(ComponentType t){const char *a[]={"CLIENT","CDN","GATEWAY","LOAD_BALANCER","AUTH","SERVICE","DATABASE","CACHE","QUEUE","WORKER","STORAGE","EXTERNAL_API","MONITOR","BACKUP","UNKNOWN"};return a[t];}
static const char *state_name(State s){const char *a[]={"RUNNING","STOPPED","STANDBY","FAILED"};return a[s];}
static const char *crit_name(Criticality c){const char *a[]={"LOW","MEDIUM","HIGH","CRITICAL"};return a[c];}
static const char *runtime_name(RuntimeStatus s){const char *a[]={"HEALTHY","DEGRADED","AFFECTED","FAILED"};return a[s];}
static const char *analysis_status_name(RuntimeStatus s){const char *a[]={"HEALTHY","DEGRADED","CRITICAL","FAILED"};return a[s];}
static int find_component(const Program *p,const char *name){for(int i=0;i<p->component_count;i++)if(!strcmp(p->components[i].name,name))return i;return -1;}
static Recovery *find_recovery(Program *p,const char *name){for(int i=0;i<p->recovery_count;i++)if(!strcmp(p->recoveries[i].failed,name))return &p->recoveries[i];return NULL;}
static void print_json_components(const SystemModel *m,int post){
    putchar('[');
    for(int i=0;i<m->program.component_count;i++){
        if(i)putchar(',');
        const RuntimeComponent *x=&m->nodes[i]; const Component *c=&x->c;
        const char *st=runtime_name(x->status);
        if(post && x->status!=R_FAILED && x->recovered) st="RECOVERED";
        else if(post && c->state==STATE_RUNNING && m->program.components[i].state==STATE_STANDBY) st="RECOVERED";
        printf("{\"name\":");js(c->name);printf(",\"type\":");js(type_name(c->type));printf(",\"state\":");js(state_name(c->state));printf(",\"criticality\":");js(crit_name(c->criticality));printf(",\"group\":");js(c->has_group?c->group:"");printf(",\"status\":");js(st);printf(",\"distance\":%d,\"reason\":",x->direct_impact?1:x->indirect_impact?2:x->failed?0:-1);js(x->failed?"FAILURE":x->direct_impact?"DIRECT":x->indirect_impact?"INDIRECT":x->optional_impact?"OPTIONAL DEPENDENCY":x->recovered?"REDIRECTED":"");printf(",\"redirect\":");js(x->active_redirect);putchar('}');
    }
    putchar(']');
}
static void print_json_model(const Program *p){
    printf("\"model\":{\"system\":");js(p->system_name);printf(",\"groups\":[");
    for(int i=0;i<p->group_count;i++){if(i)putchar(',');js(p->groups[i]);}printf("],\"components\":[");
    for(int i=0;i<p->component_count;i++){const Component *c=&p->components[i];if(i)putchar(',');printf("{\"name\":");js(c->name);printf(",\"type\":");js(type_name(c->type));printf(",\"state\":");js(state_name(c->state));printf(",\"criticality\":");js(crit_name(c->criticality));printf(",\"group\":");js(c->has_group?c->group:"");putchar('}');}printf("],\"deps\":[");
    int first=1;for(int i=0;i<p->depends_count;i++){if(!first)putchar(',');first=0;printf("{\"source\":");js(p->depends[i].source);printf(",\"target\":");js(p->depends[i].target);printf(",\"kind\":\"mandatory\"}");}
    for(int i=0;i<p->optional_count;i++){if(!first)putchar(',');first=0;printf("{\"source\":");js(p->optional[i].source);printf(",\"target\":");js(p->optional[i].target);printf(",\"kind\":\"optional\"}");}printf("],\"connects\":[");
    for(int i=0;i<p->connection_count;i++){if(i)putchar(',');printf("{\"source\":");js(p->connects[i].source);printf(",\"target\":");js(p->connects[i].target);printf(",\"kind\":\"connection\"}");}printf("],\"scenarios\":[");
    for(int i=0;i<p->scenario_count;i++){if(i)putchar(',');printf("{\"name\":");js(p->scenarios[i].name);printf(",\"failures\":[");for(int j=0;j<p->scenarios[i].failure_count;j++){if(j)putchar(',');js(p->scenarios[i].failures[j]);}printf("]}");}printf("],\"recoveries\":{");
    for(int i=0;i<p->recovery_count;i++){const Recovery *r=&p->recoveries[i];if(i)putchar(',');js(r->failed);printf(":{\"failed\":");js(r->failed);printf(",\"backup\":");if(r->has_backup)js(r->backup);else fputs("null",stdout);printf(",\"redirects\":[");for(int j=0;j<r->redirect_count;j++){if(j)putchar(',');printf("{\"source\":");js(r->redirects[j].source);printf(",\"target\":");js(r->redirects[j].target);putchar('}');}printf("]}");}printf("}}");
}
static void print_json_result(const Program *p,const Scenario *s,const SystemModel *before,const AnalysisResult *a,const SystemModel *after){
    printf("{\"ok\":true,\"compiler\":{\"ok\":true,\"stages\":[\"Lexer\",\"Parser + AST\",\"Semantic analysis\",\"System model / IR\"]},");print_json_model(p);
    printf(",\"simulation\":{\"components\":");print_json_components(before,0);printf(",\"analysis\":{\"failed\":%d,\"direct\":%d,\"indirect\":%d,\"optional\":%d,\"affected\":%d,\"groups\":%d,\"impact\":%.1f,\"status\":",a->failed_count,a->direct_count,a->indirect_count,a->optional_count,a->affected_count,a->affected_groups,a->impact_percent);js(analysis_status_name(a->system_status));printf("},\"paths\":[]},\"recovery\":{\"components\":");print_json_components(after,1);printf(",\"actions\":[");int first=1;int covered_critical=0;
    for(int f=0;f<s->failure_count;f++){Recovery *r=find_recovery((Program*)p,s->failures[f]);if(!r)continue;int has_backup=0,has_redirect=0;if(r->has_backup){if(!first)putchar(',');first=0;printf("\"Backup activated: %s\"",r->backup);has_backup=1;}for(int j=0;j<r->redirect_count;j++){if(!first)putchar(',');first=0;printf("\"Redirect: %s → %s\"",r->redirects[j].source,r->redirects[j].target);has_redirect=1;}int ix=find_component(p,s->failures[f]);if(ix>=0&&p->components[ix].criticality==CRIT_CRITICAL&&has_backup&&has_redirect)covered_critical++;}
    int remaining=0,uncovered_critical=0;for(int i=0;i<p->component_count;i++)if(after->nodes[i].status==R_FAILED||after->nodes[i].failed){remaining++;if(after->nodes[i].c.criticality==CRIT_CRITICAL){int covered=0;for(int f=0;f<s->failure_count;f++)if(!strcmp(s->failures[f],after->nodes[i].c.name)){Recovery *r=find_recovery((Program*)p,s->failures[f]);covered=(r&&r->has_backup&&r->redirect_count>0);}if(!covered)uncovered_critical++;}}
    printf("],\"recovered\":[");first=1;for(int i=0;i<p->component_count;i++)if(after->nodes[i].recovered){if(!first)putchar(',');first=0;js(p->components[i].name);}printf("],\"status\":");const char *post=remaining==p->component_count?"FAILED":uncovered_critical?"CRITICAL":remaining?"DEGRADED":s->failure_count?"RECOVERED":"HEALTHY";js(post);printf("},\"scenario\":");js(s->name);printf("}");
}
int main(int argc,char **argv){
    int json_mode=(argc>1&&!strcmp(argv[1],"--json"));
    const char *path=json_mode?(argc>2?argv[2]:"examples/ecommerce_v2.fail"):(argc>1?argv[1]:"examples/ecommerce_v2.fail");
    char *src=read_file(path);if(!src){if(json_mode){printf("{\"ok\":false,\"errors\":[\"Cannot open input file\"]}");}else fprintf(stderr,"Cannot open %s\n",path);return 1;}
    Lexer lx;lexer_init(&lx,src);int tokens=0;Token t;do{t=lexer_next(&lx);tokens++;token_free(&t);}while(t.type!=TOKEN_EOF);
    static Program p;char error[256]={0};
    if(!parse_program(src,&p,error,sizeof(error))){if(json_mode){printf("{\"ok\":false,\"errors\":[");js(error);printf("]}");}else printf("=== FAILSIM COMPILER + SIMULATOR ===\nInput: %s\n\n[1] LEXER: PASS (%d tokens)\n[2] PARSER + AST: FAIL\n%s\n",path,tokens,error);free(src);return 1;}
    if(!semantic_check(&p,error,sizeof(error))){if(json_mode){printf("{\"ok\":false,\"errors\":[");js(error);printf("]}");}else printf("=== FAILSIM COMPILER + SIMULATOR ===\nInput: %s\n\n[1] LEXER: PASS (%d tokens)\n[2] PARSER + AST: PASS\n[3] SEMANTIC ANALYSIS: FAIL\n%s\n",path,tokens,error);free(src);return 1;}
    if(json_mode){
        static Scenario scenario;static SystemModel before,after;AnalysisResult result;memset(&scenario,0,sizeof(scenario));snprintf(scenario.name,sizeof(scenario.name),"InteractiveFailure");
        for(int i=3;i<argc;i++){if(find_component(&p,argv[i])<0){char msg[160];snprintf(msg,sizeof(msg),"Unknown failure component: %s",argv[i]);printf("{\"ok\":false,\"errors\":[");js(msg);printf("]}");free(src);return 1;}if(scenario.failure_count<FS_MAX_FAILURES)snprintf(scenario.failures[scenario.failure_count++],FS_NAME,"%s",argv[i]);}
        model_build(&p,&before);simulate_scenario(&before,&scenario,&result);after=before;apply_recovery(&after,&scenario);print_json_result(&p,&scenario,&before,&result,&after);free(src);return 0;
    }
    printf("=== FAILSIM COMPILER + SIMULATOR ===\nInput: %s\n\n[1] LEXER: PASS (%d tokens)\n[2] PARSER + AST: PASS\n[3] SEMANTIC ANALYSIS: PASS\n",path,tokens);
    static SystemModel m;model_build(&p,&m);printf("[4] SYSTEM MODEL / IR: PASS\n");model_print(&m);
    for(int i=0;i<p.scenario_count;i++){model_build(&p,&m);AnalysisResult result;printf("\n=== SCENARIO: %s ===\n",p.scenarios[i].name);simulate_scenario(&m,&p.scenarios[i],&result);print_analysis(&m,&p.scenarios[i],&result);apply_recovery(&m,&p.scenarios[i]);print_recovery(&m,&p.scenarios[i]);}
    printf("\n=== FAILSIM COMPLETE ===\n");free(src);return 0;
}

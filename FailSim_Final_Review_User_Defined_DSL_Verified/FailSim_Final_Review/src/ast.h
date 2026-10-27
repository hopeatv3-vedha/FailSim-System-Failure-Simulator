#ifndef FAILSIM_AST_H
#define FAILSIM_AST_H

#define FS_MAX_COMPONENTS 256
#define FS_MAX_GROUPS 128
#define FS_MAX_DEPS 1024
#define FS_MAX_CONNECTIONS 1024
#define FS_MAX_RECOVERY 256
#define FS_MAX_FAILURES 256
#define FS_MAX_SCENARIOS 128
#define FS_NAME 64

typedef enum {
    TYPE_CLIENT, TYPE_CDN, TYPE_GATEWAY, TYPE_LOAD_BALANCER, TYPE_AUTH,
    TYPE_SERVICE, TYPE_DATABASE, TYPE_CACHE, TYPE_QUEUE, TYPE_WORKER,
    TYPE_STORAGE, TYPE_EXTERNAL_API, TYPE_MONITOR, TYPE_BACKUP, TYPE_UNKNOWN
} ComponentType;

typedef enum {
    STATE_RUNNING, STATE_STOPPED, STATE_STANDBY, STATE_FAILED
} State;

typedef enum {
    CRIT_LOW, CRIT_MEDIUM, CRIT_HIGH, CRIT_CRITICAL
} Criticality;

typedef struct {
    char name[FS_NAME];
    ComponentType type;
    State state;
    Criticality criticality;
    char group[FS_NAME];
    int has_state, has_criticality, has_group;
} Component;

typedef struct { char source[FS_NAME], target[FS_NAME]; } Relation;

typedef struct {
    char failed[FS_NAME];
    char backup[FS_NAME];
    int has_backup;
    Relation redirects[FS_MAX_COMPONENTS];
    int redirect_count;
} Recovery;

typedef struct {
    char name[FS_NAME];
    char failures[FS_MAX_FAILURES][FS_NAME];
    int failure_count;
} Scenario;

typedef struct {
    char system_name[FS_NAME];
    Component components[FS_MAX_COMPONENTS];
    int component_count;
    char groups[FS_MAX_GROUPS][FS_NAME];
    int group_count;
    Relation depends[FS_MAX_DEPS];
    int depends_count;
    Relation optional[FS_MAX_DEPS];
    int optional_count;
    Relation connects[FS_MAX_CONNECTIONS];
    int connection_count;
    Recovery recoveries[FS_MAX_RECOVERY];
    int recovery_count;
    Scenario scenarios[FS_MAX_SCENARIOS];
    int scenario_count;
} Program;

void program_init(Program *p);

#endif

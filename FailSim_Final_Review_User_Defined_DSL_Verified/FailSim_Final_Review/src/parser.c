#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { Lexer lexer; Token current; char *error; int error_size; } Parser;

static void next(Parser *p) { token_free(&p->current); p->current = lexer_next(&p->lexer); }

static int fail(Parser *p, const char *msg) {
    if (p->error && p->error_size > 0)
        snprintf(p->error, (size_t)p->error_size, "line %d: %s (got %s)", p->current.line, msg, token_type_name(p->current.type));
    return 0;
}

static int accept(Parser *p, TokenType t) {
    if (p->current.type == t) { next(p); return 1; }
    return 0;
}

static int expect(Parser *p, TokenType t, const char *msg) {
    if (!accept(p, t)) return fail(p, msg);
    return 1;
}

static int copy_name(Parser *p, char *dst) {
    if (p->current.type != TOKEN_IDENTIFIER) return fail(p, "expected identifier");
    snprintf(dst, FS_NAME, "%s", p->current.lexeme);
    next(p);
    return 1;
}

static ComponentType component_type(TokenType t) {
    switch (t) {
        case TOKEN_CLIENT: return TYPE_CLIENT; case TOKEN_CDN: return TYPE_CDN;
        case TOKEN_GATEWAY: return TYPE_GATEWAY; case TOKEN_LOAD_BALANCER: return TYPE_LOAD_BALANCER;
        case TOKEN_AUTH: return TYPE_AUTH; case TOKEN_SERVICE: return TYPE_SERVICE;
        case TOKEN_DATABASE: return TYPE_DATABASE; case TOKEN_CACHE: return TYPE_CACHE;
        case TOKEN_QUEUE: return TYPE_QUEUE; case TOKEN_WORKER: return TYPE_WORKER;
        case TOKEN_STORAGE: return TYPE_STORAGE; case TOKEN_EXTERNAL_API: return TYPE_EXTERNAL_API;
        case TOKEN_MONITOR: return TYPE_MONITOR; case TOKEN_BACKUP: return TYPE_BACKUP;
        default: return TYPE_UNKNOWN;
    }
}

static State state_value(TokenType t) {
    switch (t) {
        case TOKEN_RUNNING: return STATE_RUNNING; case TOKEN_STOPPED: return STATE_STOPPED;
        case TOKEN_STANDBY: return STATE_STANDBY; case TOKEN_FAILED: return STATE_FAILED;
        default: return STATE_RUNNING;
    }
}

static Criticality crit_value(TokenType t) {
    switch (t) {
        case TOKEN_LOW: return CRIT_LOW; case TOKEN_MEDIUM: return CRIT_MEDIUM;
        case TOKEN_HIGH: return CRIT_HIGH; case TOKEN_CRITICAL: return CRIT_CRITICAL;
        default: return CRIT_LOW;
    }
}

static int parse_component(Parser *p, Program *pr) {
    next(p); /* COMPONENT */
    if (pr->component_count >= FS_MAX_COMPONENTS) return fail(p, "too many components");
    Component *c = &pr->components[pr->component_count];
    memset(c, 0, sizeof(*c));
    c->state = STATE_RUNNING; c->criticality = CRIT_MEDIUM;
    if (!copy_name(p, c->name)) return 0;
    if (!expect(p, TOKEN_TYPE, "expected TYPE")) return 0;
    c->type = component_type(p->current.type);
    if (c->type == TYPE_UNKNOWN) return fail(p, "expected component type");
    next(p);
    while (p->current.type != TOKEN_END && p->current.type != TOKEN_EOF) {
        if (accept(p, TOKEN_STATE)) {
            if (p->current.type < TOKEN_RUNNING || p->current.type > TOKEN_FAILED) return fail(p, "expected state");
            c->state = state_value(p->current.type); c->has_state = 1; next(p);
        } else if (accept(p, TOKEN_CRITICALITY)) {
            if (p->current.type < TOKEN_LOW || p->current.type > TOKEN_CRITICAL) return fail(p, "expected criticality");
            c->criticality = crit_value(p->current.type); c->has_criticality = 1; next(p);
        } else if (accept(p, TOKEN_GROUP)) {
            if (!copy_name(p, c->group)) return 0;
            c->has_group = 1;
        } else return fail(p, "unexpected component statement");
    }
    if (!expect(p, TOKEN_END, "expected END after COMPONENT")) return 0;
    pr->component_count++;
    return 1;
}

static int parse_relation(Parser *p, Relation *r) {
    if (!copy_name(p, r->source)) return 0;
    if (!expect(p, TOKEN_ON, "expected ON")) return 0;
    if (!copy_name(p, r->target)) return 0;
    return 1;
}

static int parse_connection(Parser *p, Program *pr) {
    next(p); Relation *r = &pr->connects[pr->connection_count++];
    if (!copy_name(p, r->source)) return 0;
    if (!expect(p, TOKEN_TO, "expected TO")) return 0;
    return copy_name(p, r->target);
}

static int parse_recovery(Parser *p, Program *pr) {
    next(p);
    if (pr->recovery_count >= FS_MAX_RECOVERY) return fail(p, "too many recovery policies");
    Recovery *r = &pr->recoveries[pr->recovery_count]; memset(r, 0, sizeof(*r));
    if (!copy_name(p, r->failed)) return 0;
    while (p->current.type != TOKEN_END && p->current.type != TOKEN_EOF) {
        if (accept(p, TOKEN_USE)) {
            if (!copy_name(p, r->backup)) return 0;
            r->has_backup = 1;
        } else if (accept(p, TOKEN_REDIRECT)) {
            Relation *x = &r->redirects[r->redirect_count++];
            if (!copy_name(p, x->source)) return 0;
            if (!expect(p, TOKEN_TO, "expected TO")) return 0;
            if (!copy_name(p, x->target)) return 0;
        } else return fail(p, "unexpected recovery statement");
    }
    if (!expect(p, TOKEN_END, "expected END after RECOVERY")) return 0;
    pr->recovery_count++;
    return 1;
}

static int parse_scenario(Parser *p, Program *pr) {
    next(p);
    if (pr->scenario_count >= FS_MAX_SCENARIOS) return fail(p, "too many scenarios");
    Scenario *s = &pr->scenarios[pr->scenario_count]; memset(s, 0, sizeof(*s));
    if (!copy_name(p, s->name)) return 0;
    while (p->current.type != TOKEN_END && p->current.type != TOKEN_EOF) {
        if (accept(p, TOKEN_FAIL)) {
            if (s->failure_count >= FS_MAX_FAILURES) return fail(p, "too many failures");
            if (!copy_name(p, s->failures[s->failure_count++])) return 0;
        } else return fail(p, "expected FAIL");
    }
    if (!expect(p, TOKEN_END, "expected END after SCENARIO")) return 0;
    pr->scenario_count++;
    return 1;
}

int parse_program(const char *source, Program *program, char *error, int error_size) {
    Parser p;
    lexer_init(&p.lexer, source);
    p.current = lexer_next(&p.lexer);
    p.error = error; p.error_size = error_size;
    program_init(program);

    if (!expect(&p, TOKEN_SYSTEM, "expected SYSTEM")) return 0;
    if (!copy_name(&p, program->system_name)) return 0;

    while (p.current.type != TOKEN_EOF) {
        switch (p.current.type) {
            case TOKEN_GROUP:
                next(&p);
                if (program->group_count >= FS_MAX_GROUPS) return fail(&p, "too many groups");
                if (!copy_name(&p, program->groups[program->group_count++])) return 0;
                break;
            case TOKEN_COMPONENT: if (!parse_component(&p, program)) return 0; break;
            case TOKEN_DEPENDS:
            case TOKEN_OPTIONAL: {
                int optional = p.current.type == TOKEN_OPTIONAL; next(&p);
                Relation *r = optional ? &program->optional[program->optional_count++] : &program->depends[program->depends_count++];
                if (!parse_relation(&p, r)) return 0;
                break;
            }
            case TOKEN_CONNECTS:
                if (program->connection_count >= FS_MAX_CONNECTIONS) return fail(&p, "too many connections");
                if (!parse_connection(&p, program)) return 0;
                break;
            case TOKEN_RECOVERY: if (!parse_recovery(&p, program)) return 0; break;
            case TOKEN_SCENARIO: if (!parse_scenario(&p, program)) return 0; break;
            default: return fail(&p, "unexpected top-level declaration");
        }
    }
    token_free(&p.current);
    return 1;
}

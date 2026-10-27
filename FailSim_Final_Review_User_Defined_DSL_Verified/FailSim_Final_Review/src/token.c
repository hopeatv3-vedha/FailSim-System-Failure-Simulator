#include "token.h"

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_SYSTEM: return "SYSTEM";
        case TOKEN_COMPONENT: return "COMPONENT";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_DEPENDS: return "DEPENDS";
        case TOKEN_OPTIONAL: return "OPTIONAL";
        case TOKEN_ON: return "ON";
        case TOKEN_CONNECTS: return "CONNECTS";
        case TOKEN_TO: return "TO";
        case TOKEN_STATE: return "STATE";
        case TOKEN_RECOVERY: return "RECOVERY";
        case TOKEN_USE: return "USE";
        case TOKEN_REDIRECT: return "REDIRECT";
        case TOKEN_SCENARIO: return "SCENARIO";
        case TOKEN_FAIL: return "FAIL";
        case TOKEN_END: return "END";
        case TOKEN_GROUP: return "GROUP";
        case TOKEN_CRITICALITY: return "CRITICALITY";
        case TOKEN_CLIENT: return "CLIENT";
        case TOKEN_CDN: return "CDN";
        case TOKEN_GATEWAY: return "GATEWAY";
        case TOKEN_LOAD_BALANCER: return "LOAD_BALANCER";
        case TOKEN_AUTH: return "AUTH";
        case TOKEN_SERVICE: return "SERVICE";
        case TOKEN_DATABASE: return "DATABASE";
        case TOKEN_CACHE: return "CACHE";
        case TOKEN_QUEUE: return "QUEUE";
        case TOKEN_WORKER: return "WORKER";
        case TOKEN_STORAGE: return "STORAGE";
        case TOKEN_EXTERNAL_API: return "EXTERNAL_API";
        case TOKEN_MONITOR: return "MONITOR";
        case TOKEN_BACKUP: return "BACKUP";
        case TOKEN_RUNNING: return "RUNNING";
        case TOKEN_STOPPED: return "STOPPED";
        case TOKEN_STANDBY: return "STANDBY";
        case TOKEN_FAILED: return "FAILED";
        case TOKEN_LOW: return "LOW";
        case TOKEN_MEDIUM: return "MEDIUM";
        case TOKEN_HIGH: return "HIGH";
        case TOKEN_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

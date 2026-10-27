#include "ast.h"
#include <string.h>

void program_init(Program *p) {
    memset(p, 0, sizeof(*p));
}

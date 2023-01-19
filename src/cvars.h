#ifndef CVARS_H
#define CVARS_H

#include <stdbool.h>

bool cvars_init(void);
const char *cvar_get(const char *key);
bool cvar_set(const char *key, const char *val);
void cvars_cleanup(void);

#endif

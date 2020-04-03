#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdio.h>
#include "value.h"

typedef struct {
    char* name;
    builtin_fn fn;
} builtin;

void fprintval(FILE* stderr, value* value, int level, int quote);

extern builtin builtins[];

#endif

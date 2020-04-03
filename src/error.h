#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include "value.h"

extern int err_count;

void init_error(char* filename, FILE* err);

void error(int line, char* msg);

/*
 * Runtime errors.
 */

void set_error_location(char* file, int line);

void runtime_error(char* format, ...);

void lookup_error(char* name);

void apply_error(char* op, value* val1, value* val2);

#endif

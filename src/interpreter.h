#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "config.h"

void init_interpreter(BYTE* code, int code_len, char** files);

void execute();

#endif

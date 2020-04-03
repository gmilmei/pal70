#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdio.h>
#include "config.h"

void disassemble(FILE* out, BYTE* code, int code_len, char** files);

#endif

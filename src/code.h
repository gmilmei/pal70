#ifndef CODE_H
#define CODE_H

#include <stdio.h>
#include "config.h"

typedef enum {
    OP_APPLY = 1,
    OP_AUG,
    OP_BLOCKLINK,
    OP_DECLLABEL,
    OP_DECLNAME,
    OP_DECLNAMES,
    OP_DIV,
    OP_DUMMY,
    OP_EQ,
    OP_EQU,
    OP_FALSE,
    OP_FORMCLOSURE,
    OP_FORMLVALUE,
    OP_FORMRVALUE,
    OP_GE,
    OP_GOTO,
    OP_GR,
    OP_INITNAME,
    OP_INITNAMES,
    OP_JJ,
    OP_JUMP,
    OP_JUMPF,
    OP_LABEL,
    OP_LE,
    OP_LOADE,
    OP_LOADF,
    OP_LOADGUESS,
    OP_LOADL,
    OP_LOADN,
    OP_LOADR,
    OP_LOADS,
    OP_LOGAND,
    OP_LOGOR,
    OP_LOSE1,
    OP_LS,
    OP_MEMBERS,
    OP_MINUS,
    OP_MULT,
    OP_NE,
    OP_NEG,
    OP_NIL,
    OP_NOT,
    OP_PARAM,
    OP_PLUS,
    OP_POS,
    OP_POWER,
    OP_RES,
    OP_RESLINK,
    OP_RESTOREE1,
    OP_RETURN,
    OP_SAVE,
    OP_SETLABES,
    OP_SETUP,
    OP_TESTEMPTY,
    OP_TRUE,
    OP_TUPLE,
    OP_UPDATE
} op;


void encode_real(REAL real, BYTE* bytes);

REAL decode_real(BYTE* bytes);

void encode_integer(INTEGER integer, BYTE* bytes);

INTEGER decode_integer(BYTE* bytes);

void encode_int(int i, BYTE* bytes);

int decode_int(BYTE* bytes);

void decode_string(BYTE* bytes, char* string, int len);

void write_code(FILE* file, BYTE* bytes, int len, char** files, int files_len);

BYTE* read_code(FILE* file, int* len, char*** files, int* files_len);

#endif

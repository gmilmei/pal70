#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>

typedef enum {
    T_AND,
    T_ASS,
    T_AUG,
    T_BAR,
    T_COLON,
    T_COMMA,
    T_COND,
    T_DEF,
    T_DIV,
    T_DO,
    T_DOT,
    T_DUMMY,
    T_EOF,
    T_EQ,
    T_FALSE,
    T_GE,
    T_GOTO,
    T_GR,
    T_IF,
    T_IFNOT,
    T_IFSO,
    T_IN,
    T_INT,
    T_JJ,
    T_LAMBDA,
    T_LBRACE,
    T_LBRACKET,
    T_LE,
    T_LET,
    T_LOGAND,
    T_LOGOR,
    T_LPAREN,
    T_LS,
    T_MINUS,
    T_MULT,
    T_NAME,
    T_NE,
    T_NIL,
    T_NOSHARE,
    T_NOT,
    T_PERCENT,
    T_PLUS,
    T_POWER,
    T_RBRACE,
    T_RBRACKET,
    T_REAL,
    T_REC,
    T_RES,
    T_RPAREN,
    T_SEQ,
    T_STRING,
    T_TEST,
    T_TRUE,
    T_VALDEF,
    T_VALOF,
    T_WHERE,
    T_WHILE,
    T_WITHIN
} token_type;

typedef struct {
    token_type type;
    union {
        long integer;
        double real;
        char* string;
    } data;
    int line;
    int col;
} token;

void init_scanner();

void set_scanner_input(char* filename, FILE* file);

void scan_next(token* token);

char* token_name(token_type type);

void print_token(FILE* file, token* t);

#endif

#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include "config.h"
#include "list.h"

typedef char** namelist;

typedef enum {
    S_AND,
    S_APPLY,
    S_ASS,
    S_AUG,
    S_COLON,
    S_COMMA,
    S_COND,
    S_DEF,
    S_DIV,
    S_DUMMY,
    S_EMPTY,
    S_EQ,
    S_FALSE,
    S_GE,
    S_GOTO,
    S_GR,
    S_INT,
    S_LAMBDA,
    S_LE,
    S_LET,
    S_LOGAND,
    S_LOGOR,
    S_LS,
    S_MINUS,
    S_MULT,
    S_NAME,
    S_NE,
    S_NEG,
    S_NIL,
    S_NOSHARE,
    S_NOT,
    S_PLUS,
    S_POS,
    S_POWER,
    S_REAL,
    S_REC,
    S_RES,
    S_SEQ,
    S_STRING,
    S_TRUE,
    S_VALDEF,
    S_VALOF,
    S_WHILE,
    S_WITHIN
} tree_type;

struct _tree;

typedef struct _tree {
    tree_type type;

    union {
        struct {
            int size;
            struct _tree** elements;
        }  list;

        struct {
            struct _tree* left;
            struct _tree* right;
        } operands;

        struct _tree* operand;
        char* string;
        INTEGER integer;
        REAL real;
    } arg;

    int line;
} tree;

#define tree_list_size(t) ((t)->arg.list.size)

#define tree_list_element(t, n) ((t)->arg.list.elements[n])

#define tree_operand(t) ((t)->arg.operand)

#define tree_left(t) ((t)->arg.operands.left)

#define tree_right(t) ((t)->arg.operands.right)

#define tree_string(t) ((t)->arg.string)

#define tree_integer(t) ((t)->arg.integer)

#define tree_real(t) ((t)->arg.real)

tree* tree_make(int line, tree_type type);

tree* tree_make_list(int line, tree_type type, list* list);

tree* tree_make_unary(int line, tree_type type, tree* operand);

tree* tree_make_binary(int line, tree_type type, tree* left, tree* right);

tree* tree_make_ternary(int line, tree_type type, tree* first, tree* second, tree* third);

tree* tree_make_real(int line, REAL real);

tree* tree_make_integer(int line, INTEGER integer);

tree* tree_make_name(int line, char* name);

tree* tree_make_string(int line, char* string);

void print_tree(FILE* file, tree* tree, int indent);

#endif

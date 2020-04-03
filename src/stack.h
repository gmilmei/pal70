#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include "value.h"

struct _stack {
    value* value;
    struct _stack* next;
};

typedef struct _stack stack;

#define push(_S, _V) { \
        stack* _top = GC_NEW(stack); \
        _top->next = _S; \
        _top->value = _V; \
        _S = _top; }

#define pop2(_S, _A, _B) { \
        _A = _S->value; \
        _S = _S->next; \
        _B = _S->value; \
        _S = _S->next; }

#define pop(_S, _A) { \
        _A = _S->value; \
        _S = _S->next; }

#define top(_S, _A) { \
        _A = _S->value; }

void print_stack(FILE* file, stack* S);

#endif

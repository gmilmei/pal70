#ifndef VALUE_H
#define VALUE_H

#include <stdio.h>
#include "config.h"

typedef enum {
    V_TRUE,
    V_FALSE,
    V_INTEGER,
    V_REAL,
    V_STRING,
    V_DUMMY,
    V_TUPLE,
    V_LVALUE,
    V_CLOSURE,
    V_ENV,
    V_STACK,
    V_GUESS,
    V_BUILTIN,
    V_LABEL,
    V_TUPLEMAKER,
    V_JJ
} value_type;

struct _stack;

struct _value;

typedef struct _value* (*builtin_fn)(struct _value*, struct _stack*, struct _value*);

struct _value {
    value_type type;
    union {
        INTEGER integer;
        REAL real;
        char* string;
        struct _value* value;
        struct {
            int size;
            struct _value** values;
        } tuple;
        struct {
            int name;
            struct _value* value;
            struct _value* next;
        } env;
        struct {
            int pc;
            struct _stack* stack;
            struct _value* env;
        } stack;
        struct {
            int pc;
            struct _stack* stack;
            struct _value* env;
        } jj;
        struct {
            int pc;
            struct _stack* stack;
            struct _value* env;
        } label;
        struct {
            int pc;
            struct _value* env;
        } closure;
        struct {
            char* name;
            builtin_fn fn;
        } builtin;
        struct {
            int n;
            int len;
            struct _value** values;
        } tuplemaker;
    } v;
};

typedef struct _value value;

#define value_type(_v) ((_v)->type)

#define value_is_type(_v, _t) ((_v)->type == _t)

#define value_is_types(_v1, _v2, _t) ((_v1)->type == _t && (_v2)->type == _t)

#define value_rvalue(_v) ((_v)->v.value)

#define value_integer(_v) ((_v)->v.integer)

#define value_real(_v) ((_v)->v.real)

#define value_string(_v) ((_v)->v.string)

#define value_tuple_size(_v) ((_v)->v.tuple.size)

#define value_tuple_val(_v, _i) ((_v)->v.tuple.values[_i])

value* make_value(value_type type);

value* make_integer(INTEGER integer);

value* make_real(REAL real);

value* make_string(char* string);

value* make_tuple(int size);

value* make_lvalue(value* value);

value* make_stack(int pc, value* env, struct _stack* stack);

value* make_jj(int pc, value* env, struct _stack* stack);

value* make_label(int pc, value* env, struct _stack* stack);

value* make_closure(int pc, value* env);

value* make_builtin(char* name,builtin_fn fn);

value* env_bind(int name, value* val, value* env);

value* env_lookup(int name, value* env);

int value_equal(value* value1, value* value2);

int value_compare(value* value1, value* value2);

void print_value(FILE* file, value* value);

void print_env(FILE* file, value* env);

value* copy_value(value* val);

#endif

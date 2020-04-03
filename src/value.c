#include <string.h>
#include "strings.h"
#include "value.h"
#include "gc.h"

value* make_value(value_type type)
{
    value* V = GC_NEW(value);
    V->type = type;
    return V;
}

value* make_integer(INTEGER integer)
{
    value* V = make_value(V_INTEGER);
    V->v.integer = integer;
    return V;
}

value* make_real(REAL real)
{
    value* V = make_value(V_REAL);
    V->v.real = real;
    return V;
}

value* make_string(char* string)
{
    value* V = make_value(V_STRING);
    V->v.string = string;
    return V;
}

value* make_tuple(int size)
{
    value* V = make_value(V_TUPLE);
    V->v.tuple.size = size;
    V->v.tuple.values = GC_MALLOC(size*sizeof(value*));
    return V;
}

value* make_lvalue(value* val)
{
    value* V = make_value(V_LVALUE);
    V->v.value = val;
    return V;
}

value* make_stack(int pc, value* env, struct _stack* stack)
{
    value* V = make_value(V_STACK);
    V->v.stack.pc = pc;
    V->v.stack.env = env;
    V->v.stack.stack = stack;
    return V;
}

value* make_jj(int pc, value* env, struct _stack* stack)
{
    value* V = make_value(V_JJ);
    V->v.jj.pc = pc;
    V->v.jj.env = env;
    V->v.jj.stack = stack;
    return V;
}

value* make_label(int pc, value* env, struct _stack* stack)
{
    value* V = make_value(V_LABEL);
    V->v.label.pc = pc;
    V->v.label.env = env;
    V->v.label.stack = stack;
    return V;
}

value* make_closure(int pc, value* env)
{
    value* V = make_value(V_CLOSURE);
    V->v.closure.pc = pc;
    V->v.closure.env = env;
    return V;
}

value* make_builtin(char* name, builtin_fn fn)
{
    value* V = make_value(V_BUILTIN);
    V->v.builtin.name = name;
    V->v.builtin.fn = fn;
    return V;
}

value* env_bind(int name, value* val, value* env)
{
    value *E = make_value(V_ENV);
    E->v.env.name = name;
    E->v.env.value = val;
    E->v.env.next = env;
    return E;
}

value* env_lookup(int name, value* env)
{
    while (env) {
        if (env->v.env.name == name)
            return env->v.env.value;
        else
            env = env->v.env.next;
    }
    return 0;
}

int value_equal(value* value1, value* value2)
{
    switch (value_type(value1)) {
    case V_TRUE:
        if (value_is_type(value2, V_TRUE))
            return 1;
        else
            return 0;
    case V_FALSE:
        if (value_is_type(value2, V_FALSE))
            return 0;
        else
            return 1;
    case V_INTEGER:
        if (value_is_type(value2, V_INTEGER))
            return value_integer(value1) == value_integer(value2);
        else
            return 0;
    case V_REAL:
        if (value_is_type(value2, V_REAL))
            return value_real(value1) == value_real(value2);
        else
            return 0;
        break;
    case V_STRING:
        if (value_is_type(value2, V_STRING))
            return strcmp(value_string(value1), value_string(value2)) == 0;
        else
            return 0;
    default:
        return 0;
    }
}

int value_compare(value* value1, value* value2)
{
    switch (value_type(value1)) {
    case V_INTEGER:
        if (value_is_type(value2, V_INTEGER)) {
            INTEGER i1 = value_integer(value1);
            INTEGER i2 = value_integer(value2);
            if (i1 < i2)
                return -1;
            else if (i1 > i2)
                return 1;
            else
                return 0;
        }
        break;
    case V_REAL:
        if (value_is_type(value2, V_REAL)) {
            REAL r1 = value_real(value1);
            REAL r2 = value_real(value2);
            if (r1 < r2)
                return -1;
            else if (r1 > r2)
                return 1;
            else
                return 0;
        }
        break;
    default:
        return -2;
    }
    return -2;
}

void print_value(FILE* file, value* value)
{
    switch (value->type) {
    case V_INTEGER:
        fprintf(file, "INTEGER = %ld", value->v.integer);
        break;
    case V_REAL:
        fprintf(file, "REAL = %f", value->v.real);
        break;
    case V_TRUE:
        fprintf(file, "TRUE");
        break;
    case V_FALSE:
        fprintf(file, "FALSE");
        break;
    case V_DUMMY:
        fprintf(file, "DUMMY");
        break;
    case V_STRING:
        fprintf(file, "STRING = %s", value->v.string);
        break;
    case V_TUPLE:
        fprintf(file, "TUPLE = (");
        for (int i = 0; i < value->v.tuple.size; i++) {
            if (i > 0) fprintf(file, ", ");
            print_value(file, value->v.tuple.values[i]);
        }
        fprintf(file, ")");
        break;
    case V_LVALUE:
        fprintf(file, "LVALUE>");
        print_value(file, value->v.value);
        break;
    case V_CLOSURE:
        fprintf(file, "CLOSURE");
        break;
    case V_ENV:
        fprintf(file, "ENV");
        break;
    case V_STACK:
        fprintf(file, "STACK");
        break;
    case V_GUESS:
        fprintf(file, "GUESS");
        break;
    case V_LABEL:
        fprintf(file, "LABEL");
        break;
    case V_TUPLEMAKER:
        fprintf(file, "TUPLEMAKER");
        break;
    case V_BUILTIN:
        fprintf(file, "BUILTIN %s", value->v.builtin.name);
        break;
    case V_JJ:
        fprintf(file, "JJ");
        break;
    }
}

void print_env(FILE* file, value* env)
{
    while (env && value_is_type(env, V_ENV)) {
        int n = env->v.env.name;
        if (n >= 0) {
            value* value = env->v.env.value;
            char* name = ref_to_string(n);
            fprintf(file, "%s: ", name);
            print_value(file, value);
            fprintf(file, "\n");
        }
        env = env->v.env.next;
    }
}

typedef struct _copy_map {
    value* old;
    value* new;
    struct _copy_map* next;

} copy_map;

static value* copy_lookup(value* val, copy_map* map)
{
    while (map) {
        if (map->old == val)
            return map->new;
        else
            map = map->next;
    }
    return 0;
}

static copy_map* copy_add(value* val, value* new, copy_map* map)
{
    copy_map* new_map = GC_NEW(copy_map);
    new_map->next = map;
    new_map->old = val;
    new_map->new = new;
    return new_map;
}

static value* copy_rec(value* val, copy_map* map)
{
    if (!val) return 0;
    value* new = copy_lookup(val, map);
    if (new) return new;
    switch (value_type(val)) {
    case V_LVALUE: {
        new = make_lvalue(0);
        map = copy_add(val, new, map);
        new->v.value = copy_rec(value_rvalue(val), map);
        return new;
    }
    case V_TUPLE: {
        int size = value_tuple_size(val);
        new = make_tuple(size);
        map = copy_add(val, new, map);
        for (int i = 0; i < size; i++) {
            value_tuple_val(new, i) = copy_rec(value_tuple_val(val, i), map);
        }
        return new;
    }
    case V_TUPLEMAKER: {
        int n = val->v.tuplemaker.n;
        int len = val->v.tuplemaker.len;
        new = make_value(V_TUPLEMAKER);
        new->v.tuplemaker.len = len;
        new->v.tuplemaker.n = n;
        new->v.tuplemaker.values = GC_MALLOC(n*sizeof(value*));
        map = copy_add(val, new, map);
        for (int i = 0; i < n; i++) {
            value* tmp = copy_rec(val->v.tuplemaker.values[i], map);
            new->v.tuplemaker.values[i] = tmp;
        }
        return new;
    }
    default:
        return val;
    }
}

value* copy_value(value* val)
{
    return copy_rec(val, 0);
}

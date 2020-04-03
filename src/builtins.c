#include <string.h>
#include <gc.h>
#include "builtins.h"
#include "stack.h"
#include "error.h"
#include "strings.h"

#define in(_V) value_rvalue(_V)
#define out(_V) make_lvalue(_V)

static inline value* is(value* val, value_type type)
{
    if (value_is_type(val, type))
        return make_value(V_TRUE);
    else
        return make_value(V_FALSE);
}

static inline value* is2(value* val, value_type type1, value_type type2)
{
    if (value_is_type(val, type1) || value_is_type(val, type2))
        return make_value(V_TRUE);
    else
        return make_value(V_FALSE);
}

static value* atom(value* val, stack* S, value* E)
{
    val = in(val);
    switch (value_type(val)) {
    case V_TRUE:
    case V_FALSE:
    case V_INTEGER:
    case V_REAL:
    case V_STRING:
        return out(make_value(V_TRUE));
    default:
        return out(make_value(V_FALSE));
    }
}

static value* conc(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_TUPLE) || value_tuple_size(val) != 2) {
        apply_error("Conc", val, 0);
        return out(make_string(""));
    }
    value* A = value_rvalue(value_tuple_val(val, 0));
    value* B = value_rvalue(value_tuple_val(val, 1));
    if (!value_is_type(A, V_STRING)) {
        apply_error("Conc", val, 0);
        return out(make_string(""));
    }
    if (!value_is_type(B, V_STRING)) {
        apply_error("Conc", val, 0);
        return out(make_string(""));
    }
    char* a = value_string(A);
    int alen = strlen(a);
    char* b = value_string(B);
    int blen = strlen(b);
    char* s = GC_MALLOC(sizeof(char)*(alen+blen+1));
    s[0] = 0;
    strcat(s, a);
    strcat(s, b);
    return out(make_string(s));
}

static value* cy(value* val, stack* S, value* E)
{
    return copy_value(val);
}

static value* isdummy(value* val, stack* S, value* E)
{
    return out(is(in(val), V_DUMMY));
}

static value* isfunction(value* val, stack* S, value* E)
{
    return out(is2(in(val), V_CLOSURE, V_BUILTIN));
}

static value* islabel(value* val, stack* S, value* E)
{
    return out(is(in(val), V_LABEL));
}

static value* isnumber(value* val, stack* S, value* E)
{
    return out(is(in(val), V_INTEGER));
}

static value* isprogramclosure(value* val, stack* S, value* E)
{
    return out(is(in(val), V_CLOSURE));
}

static value* isreal(value* val, stack* S, value* E)
{
    return out(is(in(val), V_REAL));
}

static value* isstring(value* val, stack* S, value* E)
{
    return out(is(in(val), V_STRING));
}

static value* istruthvalue(value* val, stack* S, value* E)
{
    return out(is2(in(val), V_FALSE, V_TRUE));
}

static value* istuple(value* val, stack* S, value* E)
{
    return out(is(in(val), V_TUPLE));
}

static value* itor(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_INTEGER)) {
        apply_error("ItoR", val, 0);
        return out(make_real(0));
    }
    else {
        return out(make_real((REAL)value_integer(val)));
    }
}

static value* length(value* val, stack* S, value* E)
{
    val = in(val);
    int n = 0;
    if (value_is_type(val, V_TUPLE)) {
        n = value_tuple_size(val);
    }
    else {
        apply_error("Length", val, 0);
    }
    return out(make_integer(n));
}

static value* lookupinj(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_TUPLE) || value_tuple_size(val) != 2) {
        apply_error("LookupinJ", val, 0);
        return out(make_tuple(0));
    }
    value* A = value_rvalue(value_tuple_val(val, 0));
    value* B = value_rvalue(value_tuple_val(val, 1));
    if (!value_is_type(A, V_STRING)) {
         apply_error("LookupinJ", val, 0);
        return out(make_tuple(0));
    }
    if (!value_is_type(B, V_JJ)) {
         apply_error("LookupinJ", val, 0);
        return out(make_tuple(0));
    }
    int n = string_to_ref_if_exists(value_string(A));
    if (n >= 0) {
        val = env_lookup(n, B->v.jj.env);
        if (val) return val;
    }
    return out(make_tuple(0));
}

static value* null(value* val, stack* S, value* E)
{
    val = in(val);
    if (value_is_type(val, V_TUPLE) && value_tuple_size(val) == 0)
        return out(make_value(V_TRUE));
    else
        return out(make_value(V_FALSE));
}

void fprintval(FILE* file, value* val, int level, int quote)
{
    switch (value_type(val)) {
    case V_INTEGER:
        fprintf(file, "%ld", value_integer(val));
        break;
    case V_REAL:
        fprintf(file, "%f", value_real(val));
        break;
    case V_TRUE:
        fprintf(file, "true");
        break;
    case V_FALSE:
        fprintf(file, "false");
        break;
    case V_LVALUE:
        fprintval(file, val->v.value, level, quote);
        break;
    case V_BUILTIN:
        fprintf(file, "*builtin*");
        break;
    case V_CLOSURE:
        fprintf(file, "*closure*");
        break;
    case V_DUMMY:
        fprintf(file, "*dummy*");
        break;
    case V_JJ:
        fprintf(file, "*jj*");
        break;
    case V_TUPLEMAKER:
        fprintf(file, "*tuple*");
        break;
    case V_TUPLE: {
        int n = value_tuple_size(val);
        if (n == 0) {
            fprintf(file, "nil");
        }
        else {
            fprintf(file, "(");
            if (level < 10) {
                for (int i = 0; i < n; i++) {
                    if (i > 0) fprintf(file, ", ");
                    fprintval(file, value_tuple_val(val, i), level+1, quote);
                }
            }
            else {
                fprintf(file, "...");
            }
            fprintf(file, ")");
        }
        break;
    }
    case V_STRING: {
        char* s = value_string(val);
        if (quote) {
            fprintf(file, "'");
            while (*s) {
                switch (*s) {
                case '\n':
                    fprintf(file, "*n");
                    break;
                case '\t':
                    fprintf(file, "*t");
                    break;
                case '\b':
                    fprintf(file, "*b");
                    break;
                case '\'':
                    fprintf(file, "*'");
                    break;
                case '*':
                    fprintf(file, "**");
                    break;
                default:
                    fprintf(file, "%c", *s);
                    break;
                }
                s++;
            }
            fprintf(file, "'");
        }
        else {
            fprintf(file, "%s", s);
        }
        break;
    }
    default:
        fprintf(file, "$$$");
        break;
    }
}

static value* print(value* val, stack* S, value* E)
{
    fprintval(stdout, in(val), 0, 0);
    return out(make_value(V_DUMMY));
}

static value* readch(value* val, stack* S, value* E)
{
    int ch = fgetc(stdin);
    if (ch == EOF) return out(make_tuple(0));
    char* s = GC_MALLOC(2*sizeof(char));
    sprintf(s, "%c", ch);
    return out(make_string(s));
}

static value* rtoi(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_REAL)) {
        apply_error("RtoI", val, 0);
        return out(make_integer(0));
    }
    return out(make_integer((INTEGER)value_real(val)));
}

static value* share(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_TUPLE) || value_tuple_size(val) != 2) {
        apply_error("Share", val, 0);
        return out(make_value(V_FALSE));
    }
    if (value_tuple_val(val, 0) == value_tuple_val(val, 1))
        return out(make_value(V_TRUE));
    else
        return out(make_value(V_FALSE));
}

static value* stem(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_STRING)) {
        apply_error("Stem", val, 0);
        return out(make_string(""));
    }
    char* string = value_string(val);
    int len = strlen(string);
    if (len == 0) {
        apply_error("Stem", val, 0);
        return out(make_string(""));
    }
    char* s = GC_MALLOC(sizeof(char)*2);
    s[0] = string[0];
    s[1] = 0;
    return out(make_string(s));
}

static value* stern(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_STRING)) {
        apply_error("Stern", val, 0);
        return out(make_string(""));
    }
    char* string = value_string(val);
    int len = strlen(string);
    if (len == 0) {
        apply_error("Stern", val, 0);
        return out(make_string(""));
    }
    char* s = GC_MALLOC(sizeof(char)*len);
    strcat(s, string+1);
    return out(make_string(s));
}

static value* stoi(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_STRING)) {
        apply_error("StoI", val, 0);
        return out(make_integer(0));
    }
    char* s = value_string(val);
    INTEGER n = 0;
    INTEGER sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s && *s >= '0' && *s <= '9') {
        n = n*10+(*s-'0');
        s++;
    }
    return out(make_integer(n*sign));
}

static value* stor(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_STRING)) {
        apply_error("StoR", val, 0);
        return out(make_real(0));
    }
    char* s = value_string(val);

    REAL n = 0;
    REAL sign = 1;

    if (*s == '-') {
        sign = -1;
        s++;
    }

    while (*s && *s >= '0' && *s <= '9') {
        n = n*10+(*s-'0');
        s++;
    }

    if (*s && *s == '.') {
        s++;
        REAL f = 0;
        REAL b = 0.1;
        while (*s && *s >= '0' && *s <= '9') {
            f += b*(*s-'0');
            b /= 10;
            s++;
        }
        n += f;
    }

    return out(make_real(n*sign));
}

static value* swing(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_TUPLE) || value_tuple_size(val) != 3) {
        goto ERROR;
    }

    value* A = value_rvalue(value_tuple_val(val, 0));
    if (!value_is_type(val, V_TUPLE)) goto ERROR;
    int size = value_tuple_size(A);

    value* N = value_rvalue(value_tuple_val(val, 1));
    if (!value_is_type(N, V_INTEGER)) goto ERROR;
    INTEGER n = value_integer(N);
    if (n < 1 || n > size) goto ERROR;

    value* B = value_tuple_val(val, 2);

    value* R = make_tuple(size);
    for (int i = 0; i < size; i++) {
        value_tuple_val(R, i) = value_tuple_val(A, i);
    }
    value_tuple_val(R, n-1) = B;

    return out(R);

    ERROR:
    apply_error("Swing", val, 0);
    return out(make_tuple(0));
}

static value* tuple(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_INTEGER)) {
        apply_error("Tuple", val, 0);
        return out(make_tuple(0));
    }
    int n = value_integer(val);
    if (n < 0) n = 0;
    if (n == 0) {
        return out(make_tuple(0));
    }
    val = make_value(V_TUPLEMAKER);
    val->v.tuplemaker.len = n;
    val->v.tuplemaker.n = 0;
    val->v.tuplemaker.values = GC_MALLOC(n*sizeof(value*));
    return out(val);
}

static value* write(value* val, stack* S, value* E)
{
    val = in(val);
    if (!value_is_type(val, V_TUPLE)) {
        fprintval(stdout, in(val), 0, 0);
        return out(make_value(V_DUMMY));
    }

    int n = value_tuple_size(val);
    for (int i = 0; i < n; i++) {
        fprintval(stdout, value_rvalue(value_tuple_val(val, i)), 0, 0);
    }

    return out(make_value(V_DUMMY));
}

builtin builtins[] = {
    { "Atom", &atom },
    { "Conc", &conc },
    { "Cy", &cy },
    { "Isboolean", &istruthvalue },
    { "Isdummy", &isdummy },
    { "Isfunction", &isfunction },
    { "Isinteger", &isnumber },
    { "Islabel", &islabel },
    { "Isnumber", &isnumber },
    { "Isprogramclosure", &isprogramclosure },
    { "Isreal", &isreal },
    { "Isstring", &isstring },
    { "Istruthvalue", &istruthvalue },
    { "Istuple", &istuple },
    { "ItoR", &itor },
    { "Length", &length },
    { "LookupinJ", &lookupinj },
    { "Null", &null },
    { "Order", &length },
    { "Pr", &print },
    { "Print", &print },
    { "Readch", &readch },
    { "RtoI", &rtoi },
    { "Share", &share },
    { "Stem", &stem },
    { "Stern", &stern },
    { "StoI", &stoi },
    { "StoR", &stor },
    { "Swing", &swing },
    { "Tuple", &tuple },
    { "Write", &write },
    { 0, 0 }
};

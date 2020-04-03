#include <math.h>
#include <stdlib.h>
#include "builtins.h"
#include "code.h"
#include "config.h"
#include "error.h"
#include "gc.h"
#include "interpreter.h"
#include "stack.h"
#include "strings.h"
#include "value.h"

typedef struct {
    op op;
    union {
        INTEGER integer;
        REAL real;
        int ref;
        int n;
        int* refs;
    } args;
    int line;
    char* file;
} operation;

operation* program;
int program_len;

void init_interpreter(BYTE* code, int code_len, char** files)
{
    program = calloc(code_len, sizeof(operation));
    init_strings();
    /* references to params/labels */
    int* refv = calloc(code_len, sizeof(int));
    int refp = 0;
    int* params = calloc(code_len, sizeof(int));

    int n = 0;
    while (n < code_len) {
        op op = code[n];
        n++;
        int line = decode_int(&code[n]);
        n += 3;
        char* file = files[line>>24];
        line = line&0xFFFFFF;
        switch (op) {
        case OP_LOADN: {
            n++;
            INTEGER i = decode_integer(&code[n]);
            n += 8;
            program[program_len].op = op;
            program[program_len].args.integer = i;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
        case OP_LOADF: {
            n++;
            REAL real = decode_real(&code[n]);
            n += 8;
            program[program_len].op = op;
            program[program_len].args.real = real;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
        case OP_INITNAME:
        case OP_DECLNAME:
        case OP_DECLLABEL:
        case OP_LOADR:
        case OP_LOADL:
        case OP_LOADS: {
            n++;
            int len = decode_int(&code[n]);
            n += 4;
            char s[len+1];
            decode_string(&code[n], s, len);
            n += len;
            int ref = string_to_ref(s);
            program[program_len].op = op;
            program[program_len].args.ref = ref;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
        case OP_DECLNAMES: {
            n++;
            int len = decode_int(&code[n]);
            n += 4;
            int* refs = malloc((len+1)*sizeof(int));
            refs[0] = len;
            for (int i = 1; i <= len; i++) {
                int slen = decode_int(&code[n]);
                n += 4;
                char s[slen+1];
                decode_string(&code[n], s, slen);
                n += slen;
                int ref = string_to_ref(s);
                refs[i] = ref;
            }
            program[program_len].op = op;
            program[program_len].args.refs = refs;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
        case OP_TUPLE:
        case OP_UPDATE:
        case OP_SETLABES:
        case OP_MEMBERS:
        case OP_SETUP: {
            n++;
            int i = decode_int(&code[n]);
            n += 4;
            program[program_len].op = op;
            program[program_len].args.n = i;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
        case OP_LABEL: {
            n++;
            int L = decode_int(&code[n]);
            n += 4;
            params[L] = program_len;
            break;
        }
        case OP_PARAM: {
            n++;
            int L = decode_int(&code[n]);
            n += 4;
            refv[refp++] = L;
            refv[refp++] = program_len;
            program[program_len].op = op;
            program[program_len].args.n = 0;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
        case OP_EQU: {
            n++;
            int L = decode_int(&code[n]);
            n += 4;
            int N = decode_int(&code[n]);
            n += 4;
            params[L] = N;
            break;
        }
        default:
            n++;
            program[program_len].op = op;
            program[program_len].file = file;
            program[program_len].line = line;
            program_len++;
            break;
        }
    }

    /* resolve params */
    for (int i = 0; i < refp; i += 2) {
        program[refv[i+1]].args.n = params[refv[i]];
    }
    refp = 0;
}

static void init_builtins(builtin b[], value** env)
{
    value* E = *env;
    while (b->name) {
        int n = string_to_ref(b->name);
        value* A = make_builtin(b->name, b->fn);
        A = make_lvalue(A);
        E = env_bind(n, A, E);
        b++;
    }
    *env = E;
}

void execute()
{
    GC_INIT();

    value* guess_rvalue = make_value(V_GUESS);
    value* true_rvalue = make_value(V_TRUE);
    value* false_rvalue = make_value(V_FALSE);
    value* dummy_rvalue = make_value(V_DUMMY);
    value* nil_rvalue = make_tuple(0);
    int resname = string_to_ref("**res**");

    int pc = 0;
    int old_pc = 0;
    value* new_env = 0;
    stack* S = 0;
    value* E = env_bind(-1, dummy_rvalue, 0);
    value* A = 0;
    value* B = 0;

    init_builtins(builtins, &E);

    while (pc < program_len) {
        set_error_location(program[pc].file, program[pc].line);
        switch (program[pc].op) {
        case OP_LOADL: {
            int name = program[pc].args.ref;
            pc++;
            A = env_lookup(name, E);
            if (!A) {
                lookup_error(ref_to_string(name));
                A = make_lvalue(nil_rvalue);
            }
            push(S, A);
            break;
        }
        case OP_LOADR: {
            int name = program[pc].args.ref;
            pc++;
            A = env_lookup(name, E);
            if (!A) {
                lookup_error(ref_to_string(name));
                A = nil_rvalue;
            }
            else {
                A = value_rvalue(A);
            }
            push(S, A);
            break;
        }
        case OP_LOADE: {
            pc++;
            A = E;
            push(S, A);
            break;
        }
        case OP_LOADS: {
            int ref = program[pc].args.ref;
            pc++;
            char* string = ref_to_string(ref);
            A = make_string(string);
            push(S, A);
            break;
        }
        case OP_LOADN: {
            A = make_integer(program[pc].args.integer);
            push(S, A);
            pc++;
            break;
        }
        case OP_LOADF: {
            A = make_real(program[pc].args.real);
            push(S, A);
            pc++;
            break;
        }
        case OP_RESTOREE1: {
            pc++;
            pop(S, A);
            pop(S, E);
            push(S, A);
            break;
        }
        case OP_TRUE: {
            pc++;
            A = true_rvalue;
            push(S, A);
            break;
        }
        case OP_FALSE: {
            pc++;
            A = false_rvalue;
            push(S, A);
            break;
        }
        case OP_LOADGUESS: {
            pc++;
            A = guess_rvalue;
            A = make_lvalue(A);
            push(S, A);
            break;
        }
        case OP_NIL: {
            pc++;
            A = nil_rvalue;
            push(S, A);
            break;
        }
        case OP_DUMMY: {
            pc++;
            A = dummy_rvalue;
            push(S, A);
            break;
        }
        case OP_FORMCLOSURE: {
            pc++;
            int new_pc = program[pc].args.n;
            pc++;
            A = make_closure(new_pc, E);
            push(S, A);
            break;
        }
        case OP_FORMLVALUE: {
            pc++;
            pop(S, A);
            A = make_lvalue(A);
            push(S, A);
            break;
        }
        case OP_FORMRVALUE: {
            pc++;
            pop(S, A);
            A = value_rvalue(A);
            push(S, A);
            break;
        }
        case OP_TUPLE: {
            int n = program[pc].args.n;
            pc++;
            B = make_tuple(n);
            for (int i = 0; i < n; i++) {
                pop(S, A);
                value_tuple_val(B, i) = A;
            }
            A = B;
            push(S, A);
            break;
        }
        case OP_MEMBERS: {
            int n = program[pc].args.n;
            pc++;
            pop(S, A);
            B = value_rvalue(A);
            for (int i = n-1; i >= 0; i--) {
                push(S, value_tuple_val(B, i));
            }
            break;
        }
        case OP_NOT: {
            pc++;
            pop(S, A);
            if (A == false_rvalue) {
                A = true_rvalue;
            }
            else if (A == true_rvalue) {
                A = false_rvalue;
            }
            else {
                apply_error("not", A, 0);
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_LOGAND: {
            pc++;
            pop2(S, A, B);
            if (A == true_rvalue) {
                A = B;
            }
            else if (A == false_rvalue) {
                /* A = A */
            }
            else {
                apply_error("&", A, B);
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_LOGOR: {
            pc++;
            pop2(S, A, B);
            if (A == true_rvalue) {
                /* A = A */
            }
            else if (A == false_rvalue) {
                A = B;
            }
            else {
                apply_error("|", A, B);
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_AUG: {
            pc++;
            pop2(S, A, B);
            if (!value_is_type(A, V_TUPLE)) {
                apply_error("aug", A, B);
                A = nil_rvalue;
            }
            else {
                int n = value_tuple_size(A);
                value* T = make_tuple(n+1);
                int i;
                value_tuple_val(T, 0) = B;
                for (i = 0; i < n; i++) {
                    value_tuple_val(T, i+1) = value_tuple_val(A, i);
                }
                A = T;
            }
            push(S, A);
            break;
        }
        case OP_MULT: {
            pc++;
            pop2(S, A, B);
            if (value_is_types(A, B, V_INTEGER)) {
                A = make_integer(value_integer(A)*value_integer(B));
            }
            else if (value_is_types(A, B, V_REAL)) {
                A = make_real(value_real(A)*value_real(B));
            }
            else {
                apply_error("*", A, B);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_DIV: {
            pc++;
            pop2(S, A, B);
            if (value_is_types(A, B, V_INTEGER)) {
                if (value_integer(B) == 0) {
                    runtime_error("%s", "division by zero");
                    A = make_integer(0);
                }
                else {
                    A = make_integer(value_integer(A)/value_integer(B));
                }
            }
            else if (value_is_types(A, B, V_REAL)) {
                A = make_real(value_real(A)/value_real(B));
            }
            else {
                apply_error("/", A, B);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_PLUS: {
            pc++;
            pop2(S, A, B);
            if (value_is_types(A, B, V_INTEGER)) {
                A = make_integer(value_integer(A)+value_integer(B));
            }
            else if (value_is_types(A, B, V_REAL)) {
                A = make_real(value_real(A)+value_real(B));
            }
            else {
                apply_error("+", A, B);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_MINUS: {
            pc++;
            pop2(S, A, B);
            if (value_is_types(A, B, V_INTEGER)) {
                A = make_integer(value_integer(A)-value_integer(B));
            }
            else if (value_is_types(A, B, V_REAL)) {
                A = make_real(value_real(A)-value_real(B));
            }
            else {
                apply_error("-", A, B);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_POWER: {
            pc++;
            pop2(S, A, B);
            if (value_is_types(A, B, V_INTEGER)) {
                INTEGER base = value_integer(A);
                INTEGER expt = value_integer(B);
                INTEGER res = 1;
                if (expt < 0) {
                    apply_error("**", A, B);
                }
                else {
                    while (expt != 0) {
                        if ((expt & 1) != 0) res *= base;
                        base *= base;
                        expt >>= 1;
                    }
                }
                A = make_integer(res);
            }
            else if (value_is_types(A, B, V_REAL)) {
                REAL base = value_real(A);
                REAL expt = value_real(B);
                REAL res = exp(expt*log(base));
                A = make_real(res);
            }
            else {
                apply_error("**", A, B);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_POS: {
            pc++;
            pop(S, A);
            if (!value_is_type(A, V_INTEGER) && !value_is_type(A, V_REAL)) {
                apply_error("+", A, 0);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_NEG: {
            pc++;
            pop(S, A);
            if (value_is_type(A, V_INTEGER)) {
                A = make_integer(-value_integer(A));
            }
            else if (value_is_type(A, V_REAL)) {
                A = make_real(-value_real(A));
            }
            else {
                apply_error("-", A, 0);
                A = make_integer(0);
            }
            push(S, A);
            break;
        }
        case OP_EQ: {
            pc++;
            pop2(S, A, B);
            int res = value_equal(A, B);
            if (res == 1) {
                push(S, true_rvalue);
            }
            else {
                push(S, false_rvalue);
            }
            break;
        }
        case OP_NE: {
            pc++;
            pop2(S, A, B);
            int res = value_equal(A, B);
            if (res == 1) {
                push(S, false_rvalue);
            }
            else {
                push(S, true_rvalue);
            }
            break;
        }
        case OP_LS: {
            pc++;
            pop2(S, A, B);
            int res = value_compare(A, B);
            if (res < -1) {
                apply_error("<", A, B);
                A = false_rvalue;

            }
            else if (res < 0) {
                A = true_rvalue;
            }
            else {
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_LE: {
            pc++;
            pop2(S, A, B);
            int res = value_compare(A, B);
            if (res < -1) {
                apply_error("le", A, B);
                A = false_rvalue;
            }
            else if (res <= 0) {
                A = true_rvalue;
            }
            else {
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_GE: {
            pc++;
            pop2(S, A, B);
            int res = value_compare(A, B);
            if (res < -1) {
                apply_error("ge", A, B);
                A = false_rvalue;
            }
            else if (res >= 0) {
                A = true_rvalue;
            }
            else {
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_GR: {
            pc++;
            pop2(S, A, B);
            int res = value_compare(A, B);
            if (res < -1) {
                apply_error("gr", A, B);
                A = false_rvalue;
            }
            else if (res > 0) {
                A = true_rvalue;
            }
            else {
                A = false_rvalue;
            }
            push(S, A);
            break;
        }
        case OP_JUMP: {
            pc++;
            pc = program[pc].args.n;
             break;
        }
        case OP_JUMPF: {
            pop(S, A);
            if (value_is_type(A, V_FALSE)) {
                pc = program[pc+1].args.n;
            }
            else if (value_is_type(A, V_TRUE)) {
                pc += 2;
            }
            else {
                runtime_error("%s: %v", "not a truthvalue", A);
                pc += 2;
            }
            break;
        }
        case OP_APPLY: {
            pc++;
            pop(S, A);
            A = value_rvalue(A); /* A is an LVALUE, get RVALUE */
            switch (value_type(A)) {
            case V_CLOSURE:
                old_pc = pc;
                pc = A->v.closure.pc;
                new_env = A->v.closure.env;
                break;
            case V_TUPLE:
                pop(S, B);
                B = value_rvalue(B);
                if (!value_is_type(B, V_INTEGER)) {
                    runtime_error("%v applied to %v", A, B);
                    A = make_lvalue(nil_rvalue);
                    push(S, A);
                    break;
                }
                int n = value_integer(B);
                if (n > 0 && n <= value_tuple_size(A)) {
                    A = value_tuple_val(A, n-1);
                }
                else {
                    runtime_error("%v applied to %v", A, B);
                    A = make_lvalue(nil_rvalue);
                }
                push(S, A);
                break;
            case V_TUPLEMAKER:
                pop(S, B);
                n = A->v.tuplemaker.n;
                int len = A->v.tuplemaker.len;
                if (n < len-1) {
                    value* val = make_value(V_TUPLEMAKER);
                    val->v.tuplemaker.len = len;
                    val->v.tuplemaker.n = n+1;
                    val->v.tuplemaker.values = GC_MALLOC(len*sizeof(value*));
                    for (int i = 0; i < n; i++) {
                        val->v.tuplemaker.values[i] = A->v.tuplemaker.values[i];
                    }
                    val->v.tuplemaker.values[n] = B;
                    A = val;
                }
                else {
                    value* val = make_tuple(len);
                    for (int i = 0; i < n; i++) {
                        value_tuple_val(val, i) = value_tuple_val(A, i);
                    }
                    value_tuple_val(val, n) = B;
                    A = val;
                }
                push(S, make_lvalue(A));
                break;
            case V_BUILTIN:
                pop(S, B);
                A = A->v.builtin.fn(B, S, E);
                push(S, A);
                break;
            default:
                pop(S, B);
                runtime_error("attempt to apply %v to %v", A, B);
                push(S, B);
                break;
            }
            break;
        }
        case OP_SAVE: {
            pc++;
            pop(S, B);
            push(S, make_stack(old_pc, E, S));
            push(S, B);
            if (new_env) {
                E = new_env;
                new_env = 0;
            }
            pc++;
            break;
        }
        case OP_RETURN: {
            pop(S, A);
            value* saved;
            pop(S, saved);
            pc = saved->v.stack.pc;
            E = saved->v.stack.env;
            S = saved->v.stack.stack;
            push(S, A);
            break;
        }
        case OP_TESTEMPTY: {
            pc++;
            pop(S, A);
            if (value_rvalue(A) != nil_rvalue) {
                runtime_error("%s: %v", "function of no arguments", A);
            }
            break;
        }
        case OP_LOSE1: {
            pc++;
            pop(S, B);
            break;
        }
        case OP_GOTO: {
            pc++;
            pop(S, A);
            if (!value_is_type(A, V_LABEL)) {
                runtime_error("%s %v", "cannot go to", A);
                A = dummy_rvalue;
                break;
            }
            pc = A->v.label.pc;
            E = A->v.label.env;
            S = A->v.label.stack;
            break;
        }
        case OP_UPDATE: {
            int n = program[pc].args.n;
            /* A rvalue, B lvalue to be updated */
            pop2(S, A, B);
            if (n == 1) {
                /* one update */
                value_rvalue(B) = A;
            }
            else if (value_is_type(A, V_TUPLE) && value_tuple_size(A) == n) {
                /* multiple update */
                B = value_rvalue(B);
                value* tmp = make_tuple(n);
                for (int i = 0; i < n; i++)
                    value_tuple_val(tmp, i) = value_rvalue(value_tuple_val(A, i));
                for (int i = 0; i < n; i++)
                    value_rvalue(value_tuple_val(B, i)) = value_tuple_val(tmp, i);
            }
            else {
                runtime_error("%s", "conformality error in assignment");
            }
            A = dummy_rvalue;
            push(S, A);
            pc++;
            break;
        }
        case OP_DECLNAME: {
            pop(S, A);
            E = env_bind(program[pc].args.ref, A, E);
            pc++;
            break;
        }
        case OP_DECLNAMES: {
            int* refs = program[pc].args.refs;
            int n = refs[0];
            pc++;
            pop(S, A);
            A = value_rvalue(A);
            if (value_is_type(A, V_TUPLE) && value_tuple_size(A) == n) {
                for (int i = 0; i < n; i++) {
                    B = value_tuple_val(A, i);
                    E = env_bind(refs[i+1], B, E);
                }
            }
            else {
                runtime_error("%s", "conformality error in definition");
            }
            break;
        }
        case OP_INITNAME: {
            int name = program[pc].args.ref;
            B = env_lookup(name, E);
            if (!B) B = make_lvalue(nil_rvalue);
            pc++;
            pop(S, A);
            B->v.value = A->v.value;
            break;
        }
        case OP_INITNAMES: {
            pc++;
            // TODO: not yet implemented
            break;
        }
        case OP_DECLLABEL: {
            int label = program[pc].args.ref;
            pc++;
            int c = program[pc].args.ref;
            pc++;
            A = make_label(c, E, S);
            A = make_lvalue(A);
            E = env_bind(label, A, E);
            break;
        }
        case OP_SETLABES: {
            int n = program[pc].args.n;
            pc++;
            A = E;
            for (int i = 0; i < n; i++) {
                A->v.env.value->v.value->v.label.env = E;
                A = A->v.env.next;
            }
            break;
        }
        case OP_BLOCKLINK: {
            pc++;
            old_pc = program[pc].args.n;
            A = make_lvalue(E);
            pc++;
            break;
        }
        case OP_RESLINK: {
            A = make_lvalue(nil_rvalue);
            push(S, A);
            /* continue with blocklink */
            pc++;
            old_pc = program[pc].args.n;
            A = make_lvalue(E);
            pc++;
            break;
        }
        case OP_SETUP: {
            pc++;
            /* do nothing */
            break;
        }
        case OP_RES: {
            pc++;
            pop(S, A);
            value* jjval = env_lookup(resname, E);
            if (!jjval) jjval = make_lvalue(nil_rvalue);
            jjval = value_rvalue(jjval);
            if (!value_is_type(jjval, V_JJ)) {
                runtime_error("%s", "incorrect use of res");
                push(S, A);
                break;
            }
            pc = jjval->v.jj.pc;
            E = jjval->v.jj.env;
            S = jjval->v.jj.stack;
            push(S, A);
            break;
        }
        case OP_JJ: {
            pc++;
            // TODO: search for STACK value
            top(S, A);
            A = make_jj(A->v.stack.pc, A->v.stack.env, A->v.stack.stack);
            push(S, A);
            break;
        }
        default:
            runtime_error("%s %d", "unknown opcode", program[pc].op);
            return;
        }
    }
}

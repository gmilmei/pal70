#include <stdlib.h>
#include <string.h>
#include "translator.h"
#include "error.h"
#include "code.h"

static int param_number;
static int ssp;
static int msp;
static BYTE* code;
static int code_max;
static int code_len;
static int line;

typedef enum {
    MODE_VAL,
    MODE_REF
} trans_mode;

static void trans(tree* t, trans_mode mode);
static void declnames(tree* t);

/* set line of tree */
static void sl(tree* t) {
    line = t->line;
}

static void ensure_code(int len)
{
    if (len >= code_max) {
        code_max *= 2;
        code = realloc(code, sizeof(BYTE)*code_max);
    }
}

static op type_to_op(tree_type type)
{
    switch (type) {
    case S_DIV:    return OP_DIV;
    case S_DUMMY:  return OP_DUMMY;
    case S_EQ:     return OP_EQ;
    case S_FALSE:  return OP_FALSE;
    case S_GE:     return OP_GE;
    case S_GR:     return OP_GR;
    case S_LE:     return OP_LE;
    case S_LOGAND: return OP_LOGAND;
    case S_LOGOR:  return OP_LOGOR;
    case S_LS:     return OP_LS;
    case S_MINUS:  return OP_MINUS;
    case S_MULT:   return OP_MULT;
    case S_NE:     return OP_NE;
    case S_NEG:    return OP_NEG;
    case S_NIL:    return OP_NIL;
    case S_NOT:    return OP_NOT;
    case S_PLUS:   return OP_PLUS;
    case S_POS:    return OP_POS;
    case S_POWER:  return OP_POWER;
    case S_TRUE:   return OP_TRUE;
    default:       return 0;
    }
}

static void up_ssp(int n)
{
    ssp += n;
    if (ssp > msp) msp = ssp;
}

static int next_param()
{
    param_number++;
    return param_number;
}

static void out_byte(BYTE byte)
{
    ensure_code(code_len+1);
    code[code_len++] = byte;
}

static void out_int(int i)
{
    ensure_code(code_len+4);
    encode_int(i, &code[code_len]);
    code_len += 4;
}

static void out_integer(INTEGER integer)
{
    ensure_code(code_len+8);
    encode_integer(integer, &code[code_len]);
    code_len += 8;
}

static void out_real(REAL real)
{
    ensure_code(code_len+8);
    encode_real(real, &code[code_len]);
    code_len += 8;
}

static void out_op(op op)
{
    out_byte(op);
    out_int(line);
}

static void out_equ(int L, int N)
{
    out_op(OP_EQU);
    out_int(L);
    out_int(N);
}

static void out_param(int N)
{
    out_op(OP_PARAM);
    out_int(N);
}

static void out_name(char* s)
{
    int len = strlen(s);
    out_int(len);
    for (int i = 0; i < len; i++) {
        out_byte(s[i]);
    }
}

static void out_string(char* s)
{
    int len = strlen(s);
    out_int(len);
    for (int i = 0; i < len; i++) {
        out_byte(s[i]);
    }
}

static void out_label(int N)
{
    out_op(OP_LABEL);
    out_int(N);
}

static void load_definee(tree* t)
{
    if (!t) return;
    sl(t);
    switch (t->type) {
    case S_NAME: {
        out_op(OP_LOADR);
        out_name(tree_string(t));
        up_ssp(1);
        out_op(OP_FORMLVALUE);
        break;
    }
    case S_AND: {
        int size = tree_list_size(t);
        for (int i = size-1; i >= 0; i--) {
            load_definee(tree_list_element(t, i));
        }
        out_op(OP_TUPLE);
        out_int(size);
        ssp = ssp-size+1;
        out_op(OP_FORMLVALUE);
        break;
    }
    case S_COMMA: {
        int size = tree_list_size(t);
        for (int i = size-1; i >= 0; i--) {
            load_definee(tree_list_element(t, i));
        }
        out_op(OP_TUPLE);
        out_int(size);
        ssp = ssp-size+1;
        out_op(OP_FORMLVALUE);
        break;
    }
    case S_REC: {
        load_definee(tree_operand(t));
        break;
    }
    case S_VALDEF: {
        load_definee(tree_left(t));
        break;
    }
    case S_WITHIN: {
        load_definee(tree_right(t));
        break;
    }
    default:
        break;
    }
}

static void declguesses(tree* t)
{
    if (!t) return;
    sl(t);
    switch (t->type) {
    case S_NAME: {
        out_op(OP_LOADGUESS);
        if (ssp == msp) msp = ssp+1;
        out_op(OP_DECLNAME);
        out_name(tree_string(t));
        break;
    }
    case S_AND: {
        int size = tree_list_size(t);
        for (int i = 0; i < size; i++) {
            declguesses(tree_list_element(t, i));
        }
        break;
    }
    case S_COMMA: {
        int size = tree_list_size(t);
        for (int i = 0; i < size; i++) {
            declguesses(tree_list_element(t, i));
        }
        break;
    }
    case S_REC: {
        declguesses(tree_operand(t));
        break;
    }
    case S_VALDEF: {
        declguesses(tree_left(t));
        break;
    }
    case S_WITHIN: {
        declguesses(tree_right(t));
        break;
    }
    default:
        break;
    }
}

static void initnames(tree* t)
{
    if (!t) return;
    sl(t);
    switch (t->type) {
    case S_NAME: {
        out_op(OP_INITNAME);
        out_name(tree_string(t));
        ssp--;
        break;
    }
    case S_AND: {
        int size = tree_list_size(t);
        out_op(OP_MEMBERS);
        out_int(size);
        up_ssp(size-1);
        for (int i = 0; i < size; i++) {
            initnames(tree_list_element(t, i));
        }
        break;
    }
    case S_COMMA: {
        int size = tree_list_size(t);
        out_op(OP_INITNAMES);
        out_int(size);
        ssp--;
        for (int i = 0; i < size; i++) {
            char* name = tree_string(tree_list_element(t, i));
            out_name(name);
        }
        break;
    }
    case S_REC: {
        initnames(tree_operand(t));
        break;
    }
    case S_VALDEF: {
        initnames(tree_left(t));
        break;
    }
    case S_WITHIN: {
        initnames(tree_right(t));
        break;
    }
    default:
        break;
    }
}

/*
 * Find labels in the tree and generate label numbers.
 */
static int find_labels(tree* t)
{
    if (!t) return 0;
    sl(t);
    switch (t->type) {
    case S_COLON: {
        /* new label number */
        int L = next_param();
        /* add label number to colon statement */
        tree_list_element(t, 2) = tree_make_integer(t->line, L);
        out_op(OP_DECLLABEL);
        char* name = tree_string(tree_list_element(t, 0));
        out_name(name);
        out_param(L);
        return 1+find_labels(tree_list_element(t, 1));
    }
    case S_COND: {
        int nl1 = find_labels(tree_list_element(t, 1));
        int nl2 = find_labels(tree_list_element(t, 2));
        return nl1+nl2;
    }
    case S_WHILE:
        return find_labels(tree_right(t));
    case S_SEQ:
        return find_labels(tree_left(t))+find_labels(tree_right(t));
    default:
        return 0;
    }
}

static void trans_labels(tree* t)
{
    int n = find_labels(t);
    if (n != 0) {
        sl(t);
        out_op(OP_SETLABES);
        out_int(n);
    }
}

static void trans_rhs(tree* t)
{
    if (!t) return;
    sl(t);
    switch (t->type) {
    case S_AND: {
        int size = tree_list_size(t);
        for (int i = size-1; i >= 0; i--) {
            trans_rhs(tree_list_element(t, i));
        }
        out_op(OP_TUPLE);
        out_int(size);
        ssp = ssp-size+1;
        out_op(OP_FORMLVALUE);
        break;
    }
    case S_VALDEF:
        trans(tree_right(t), MODE_REF);
        break;
    case S_REC: {
        out_op(OP_LOADE);
        up_ssp(1);
        tree* t1 = tree_operand(t);
        declguesses(t1);
        trans_rhs(t1);
        initnames(t1);
        load_definee(t1);
        out_op(OP_RESTOREE1);
        ssp--;
        break;
    }
    case S_WITHIN: {
        int L = next_param();
        int N = next_param();
        trans_rhs(tree_left(t));
        out_op(OP_BLOCKLINK);
        out_param(L);
        if (ssp == msp) msp = ssp+1;
        int ssp_save = ssp;
        int msp_save = msp;
        ssp = 1;
        msp = 1;
        out_op(OP_SAVE);
        out_param(N);
        declnames(tree_left(t));
        trans_rhs(tree_right(t));
        out_op(OP_RETURN);
        out_equ(N, msp);
        ssp = ssp_save;
        msp = msp_save;
        out_label(L);
        break;
    }
    default:
        break;
    }
}

static void declnames(tree* t)
{
    if (!t) return;
    sl(t);
    switch (t->type) {
    case S_NAME: {
        out_op(OP_DECLNAME);
        out_name(tree_string(t));
        ssp--;
        break;
    }
    case S_COMMA: {
        int size = tree_list_size(t);
        out_op(OP_DECLNAMES);
        out_int(size);
        ssp--;
        for (int i = 0; i < size; i++) {
            char* name = tree_string(tree_list_element(t, i));
            out_name(name);
        }
        break;
    }
    case S_AND: {
        int size = tree_list_size(t);
        out_op(OP_MEMBERS);
        out_int(size);
        up_ssp(size-1);
        for (int i = 0; i < size; i++) {
            declnames(tree_list_element(t, i));
        }
        break;
    }
    case S_REC: {
        declnames(tree_operand(t));
        break;
    }
    case S_VALDEF: {
        declnames(tree_left(t));
        break;
    }
    case S_WITHIN: {
        declnames(tree_right(t));
        break;
    }
    case S_EMPTY: {
        out_op(OP_TESTEMPTY);
        ssp--;
        break;
    }
    default:
        break;
    }
}

static void trans_scope(tree* decl, tree* body, int N, trans_mode mode)
{
    int ssp_save = ssp;
    int msp_save = msp;
    ssp = 1;
    msp = 1;
    sl(decl);
    out_op(OP_SAVE);
    out_param(N);
    declnames(decl);
    sl(body);
    trans_labels(body);
    trans(body, mode);
    out_op(OP_RETURN);
    out_equ(N, msp);
    ssp = ssp_save;
    msp = msp_save;
}

static void trans(tree* t, trans_mode mode)
{
    if (!t) {
        error(0, "missing expression");
        out_op(OP_NIL);
        up_ssp(1);
        return;
    }

    sl(t);
    tree_type type = t->type;
    op op = type_to_op(type);

    switch (type) {
    case S_LET: {
        int L = next_param();
        int N = next_param();
        trans_rhs(tree_left(t));
        out_op(OP_BLOCKLINK);
        out_param(L);
        if (ssp == msp) msp = ssp+1;
        trans_scope(tree_left(t), tree_right(t), N, mode);
        out_label(L);
        break;
    }
    case S_DEF: {
        for (int i = 0; i < tree_list_size(t); i++) {
            trans_rhs(tree_list_element(t, i));
            declnames(tree_list_element(t, i));
            if (i < tree_list_size(t)-1) {
                trans_labels(tree_list_element(t, i+1));
            }
        }
        break;
    }
    case S_MULT:
    case S_DIV:
    case S_PLUS:
    case S_MINUS:
    case S_POWER:
    case S_EQ:
    case S_LS:
    case S_GR:
    case S_GE:
    case S_LE:
    case S_NE:
    case S_LOGAND:
    case S_LOGOR: {
        trans(tree_right(t), MODE_VAL);
        trans(tree_left(t), MODE_VAL);
        out_op(op);
        ssp--;
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_AUG: {
        trans(tree_right(t), MODE_REF);
        trans(tree_left(t), MODE_VAL);
        out_op(OP_AUG);
        ssp--;
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_APPLY: {
        trans(tree_right(t), MODE_REF);
        trans(tree_left(t), MODE_REF);
        sl(t);
        out_op(OP_APPLY);
        ssp--;
        if (mode == MODE_VAL) out_op(OP_FORMRVALUE);
        break;
    }
    case S_POS:
    case S_NEG:
    case S_NOT: {
        trans(tree_operand(t), MODE_VAL);
        out_op(op);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_NOSHARE: {
        trans(tree_operand(t), MODE_VAL);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_COMMA: {
        int size = tree_list_size(t);
        for (int i = size-1; i >= 0; i--) {
            /* compile components for tuples as ref */
            trans(tree_list_element(t, i), MODE_REF);
        }
        out_op(OP_TUPLE);
        out_int(size);
        ssp = ssp-size+1;
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_LAMBDA: {
        /* label for lambda body */
        int L = next_param();
        /* label to jump around body */
        int M = next_param();
        int N = next_param();

        out_op(OP_FORMCLOSURE);
        out_param(L);
        up_ssp(1);

        /* jump around lambda body */
        out_op(OP_JUMP);
        out_param(M);

        /* lambda body label */
        out_label(L);
        trans_scope(tree_left(t), tree_right(t), N, MODE_REF);

        out_label(M);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_COLON: {
        tree* label = tree_list_element(t, 2);
        int L = 0;
        if (!label)
            error(t->line, "label improperly used");
        else
            L = tree_integer(label);
        out_label(L);
        trans(tree_list_element(t, 1), mode);
        break;
    }
    case S_SEQ: {
        trans(tree_left(t), MODE_VAL);
        out_op(OP_LOSE1);
        ssp--;
        trans(tree_right(t), mode);
        break;
    }
    case S_VALOF: {
        int L = next_param();
        int N = next_param();
        out_op(OP_RESLINK);
        out_param(L);
        ssp++;
        if (ssp >= msp) msp = ssp+1;
        int ssp_save = ssp;
        int msp_save = msp;
        ssp = 0;
        msp = 1;
        out_op(OP_SAVE);
        out_param(N);
        out_op(OP_TESTEMPTY);
        out_op(OP_JJ);
        out_op(OP_FORMLVALUE);
        out_op(OP_DECLNAME);
        out_name("**res**");
        trans_labels(tree_operand(t));
        trans(tree_operand(t), MODE_REF);
        out_op(OP_RETURN);
        out_equ(N, msp);
        ssp = ssp_save;
        msp = msp_save;
        out_label(L);
        if (mode == MODE_VAL) out_op(OP_FORMRVALUE);
        break;
    }
    case S_RES: {
        trans(tree_operand(t), MODE_REF);
        out_op(OP_RES);
        break;
    }
    case S_GOTO: {
        trans(tree_operand(t), MODE_VAL);
        out_op(OP_GOTO);
        break;
    }
    case S_COND: {
        int L = next_param();
        int M = next_param();
        trans(tree_list_element(t, 0), MODE_VAL);
        out_op(OP_JUMPF);
        out_param(L);
        ssp--;
        trans(tree_list_element(t, 1), mode);
        out_op(OP_JUMP);
        out_param(M);
        out_label(L);
        ssp--;
        trans(tree_list_element(t, 2), mode);
        out_label(M);
        break;
    }
    case S_WHILE: {
        int L = next_param();
        int M = next_param();
        out_label(M);
        trans(tree_left(t), MODE_VAL);
        out_op(OP_JUMPF);
        out_param(L);
        ssp--;
        trans(tree_right(t), MODE_VAL);
        out_op(OP_LOSE1);
        out_op(OP_JUMP);
        out_param(M);
        out_label(L);
        out_op(OP_DUMMY);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_ASS: {
        tree* left = tree_left(t);
        trans(left, MODE_REF);
        trans(tree_right(t), MODE_VAL);
        out_op(OP_UPDATE);
        int n = left->type == S_COMMA?tree_list_size(left):1;
        out_int(n);
        ssp--;
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    }
    case S_NIL:
    case S_DUMMY:
    case S_TRUE:
    case S_FALSE:
        out_op(op);
        up_ssp(1);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    case S_NAME:
        if (mode == MODE_VAL)
            out_op(OP_LOADR);
        else
            out_op(OP_LOADL);
        out_name(tree_string(t));
        up_ssp(1);
        break;
    case S_INT:
        out_op(OP_LOADN);
        out_integer(tree_integer(t));
        up_ssp(1);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    case S_REAL:
        out_op(OP_LOADF);
        out_real(tree_real(t));
        up_ssp(1);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    case S_STRING:
        out_op(OP_LOADS);
        out_string(tree_string(t));
        up_ssp(1);
        if (mode == MODE_REF) out_op(OP_FORMLVALUE);
        break;
    default:
        break;
    }
}

BYTE* translate(tree* t, int* len)
{
    int n = next_param();
    ssp = 0;
    msp = 1;
    sl(t);
    out_op(OP_SETUP);
    out_int(n);
    trans_labels(t);
    trans(t, MODE_VAL);
    out_equ(n, msp);
    *len = code_len;
    return code;
}

BYTE* translate_list(list* tree_list, int* len)
{
    int n = next_param();
    ssp = 0;
    msp = 1;
    out_op(OP_SETUP);
    out_int(n);
    for (int i = 0; i < tree_list->len; i++) {
        tree* t = list_element(tree_list, i);
        sl(t);
        trans_labels(t);
        trans(t, MODE_VAL);
    }
    out_equ(n, msp);
    *len = code_len;
    return code;
}

void init_translator()
{
    line = 0;
    code_max = 1024;
    code_len = 0;
    code = malloc(sizeof(BYTE)*code_max);
    param_number = 0;
}

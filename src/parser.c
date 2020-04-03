#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "list.h"
#include "tree.h"
#include "error.h"

static token cur_tok;
static int filenr;

#define type() cur_tok.type

#define at(t) (cur_tok.type == t)

#define line() (cur_tok.line|filenr)

static list* parse_params();
static tree* parse_def_cont(int n);
static tree* parse_def(int n);
static tree* parse_com(int n);
static tree* parse_exp(int n);

static int isbracket(token_type type)
{
    return type == T_LPAREN || type == T_LBRACE || type == T_LBRACKET;
}

/*
 * Returns closing bracket for the specified opening bracket.
 */
static token_type bracket_end(token_type type)
{
    switch (type) {
    case T_LPAREN:
        return T_RPAREN;
    case T_LBRACE:
        return T_RBRACE;
    case T_LBRACKET:
        return T_RBRACKET;
    default:
        return 0;
    }
}

/* only used for operators */
static tree_type token_to_tree_type(token_type type) {
    switch (type) {
    case T_PLUS:
        return S_PLUS;
    case T_MINUS:
        return S_MINUS;
    case T_MULT:
        return S_MULT;
    case T_DIV:
        return S_DIV;
    case T_VALDEF:
        return S_EQ;
    case T_GE:
        return S_GE;
    case T_NE:
        return S_NE;
    case T_LE:
        return S_LE;
    case T_EQ:
        return S_EQ;
    case T_LS:
        return S_LS;
    case T_GR:
        return S_GR;
    default:
        return 0;
    }
}

static void next()
{
    scan_next(&cur_tok);
}

void init_parser(char* filename, FILE* file, int fn)
{
    init_scanner();
    set_scanner_input(filename, file);
    filenr = fn<<24;
    next();
}

static list* parse_name_list()
{
    list* name_list = list_new();
    list_append(name_list, cur_tok.data.string);
    next();
    while (at(T_COMMA)) {
        next();
        if (at(T_NAME)) {
            list_append(name_list, cur_tok.data.string);
            next();
        }
        else {
            error(line(), "expected name after comma");
        }
    }
    return name_list;
}

static tree* parse_names()
{
    int line = line();
    tree* res = tree_make_name(line(), cur_tok.data.string);
    next();
    if (at(T_COMMA)) {
        list* tl = list_new();
        list_append(tl, res);
        while (at(T_COMMA)) {
            next();
            if (at(T_NAME)) {
                list_append(tl, tree_make_name(line(), cur_tok.data.string));
                next();
            }
            else {
                error(line(), "expected name after comma");
            }
        }
        res = tree_make_list(line, S_COMMA, tl);
    }
    return res;
}

static tree* parse_com_cont(tree* res, int n)
{
    while (1) {
        int line = line();
        switch (type()) {
        case T_WHERE:
            if (n > 2) return res;
            next();
            return tree_make_binary(line, S_LET, parse_def_cont(0), res);
        case T_SEQ:
            if (n > 6) return res;
            next();
            res = tree_make_binary(line, S_SEQ, res, parse_com(6));
            continue;
        case T_COLON:
            if (res->type != S_NAME || n > 8) {
                error(line(), "syntax error in label");
            }
            next();
            res = tree_make_ternary(line, S_COLON, res, parse_com(8), 0);
            continue;
        default:
            return res;
        }
    }
}

static tree* parse_com(int n)
{
    tree* res;
    int line = line();
    switch (type()) {
    case T_LET: {
        if (n != 0) error(line, "'let' out of context");
        next();
        res = parse_def(0);
        if (!at(T_IN)) error(line(), "missing 'in'");
        next();
        res = tree_make_binary(line, S_LET, res, parse_com(0));
        return res;
    }
    case T_LAMBDA: {
        if (n != 0) error(line, "lambda out of context");
        next();
        list* params = parse_params();
        if (params->len == 0) error(line, "no lambda parameters");
        if (!at(T_DOT)) error(line(), "missing '.'");
        next();
        res = parse_com(0);
        for (int i = params->len-1; i >= 0; i--) {
            tree* param = list_element(params, i);
            res = tree_make_binary(line, S_LAMBDA, param, res);
        }
        return res;
    }
    case T_VALOF: {
        if (n > 4) error(line, "'valof' out of context");
        next();
        res = tree_make_unary(line, S_VALOF, parse_com(6));
        return parse_com_cont(res, n);
    }
    case T_TEST: {
        if (n > 10) error(line, "'test' out of context");
        next();
        res = parse_exp(20);
        if (at(T_IFSO)) {
            next();
            tree* ifso = parse_com(8);
            if (!at(T_IFNOT)) {
                error(line(), "missing 'ifnot'");
                return parse_com_cont(res, n);
            }
            next();
            tree* ifnot = parse_com(8);
            res = tree_make_ternary(line, S_COND, res, ifso, ifnot);
            return parse_com_cont(res, n);
        }
        else if (at(T_IFNOT)) {
            next();
            tree* ifnot = parse_com(8);
            if (!at(T_IFSO)) {
                error(line(), "missing 'ifso'");
                return parse_com_cont(res, n);
            }
            next();
            tree* ifso = parse_com(8);
            res = tree_make_ternary(line, S_COND, res, ifso, ifnot);
            return parse_com_cont(res, n);
        }
        else {
            error(line(), "missing 'ifnot' and 'ifso'");
            return parse_com_cont(res, n);
        }
    }
    case T_IF:
    case T_WHILE: {
        token_type op = type();
        if (n > 10) error(line, "'if' or 'while' out of context");
        next();
        res = parse_exp(20);
        if (at(T_DO))
            next();
        else
            error(line(), "'do' assumed to be missing");
        tree* body = parse_com(8);
        if (op == T_IF)
            res = tree_make_ternary(line, S_COND, res, body, tree_make(line, S_DUMMY));
        else
            res = tree_make_binary(line, S_WHILE, res, body);
        return parse_com_cont(res, n);
    }
    case T_GOTO: {
        next();
        res = parse_exp(38);
        res = tree_make_unary(line, S_GOTO, res);
        return parse_com_cont(res, n);
    }
    case T_RES: {
        next();
        res = parse_exp(14);
        res = tree_make_unary(line, S_RES, res);
        return parse_com_cont(res, n);
    }
    case T_DUMMY: {
        res = tree_make(line, S_DUMMY);
        next();
        return parse_com_cont(res, n);
    }
    default:
        res = parse_exp(n);
        if (!at(T_ASS)) return parse_com_cont(res, n);
        next();
        res = tree_make_binary(line, S_ASS, res, parse_exp(14));
        return parse_com_cont(res, n);
    }
}

static list* parse_params()
{
    list* params = list_new();
    while (at(T_NAME) || isbracket(type())) {
        if (at(T_NAME)) {
            list_append(params, tree_make_name(line(), cur_tok.data.string));
            next();
        }
        else if (isbracket(type())) {
            token_type end_bracket = bracket_end(type());
            next();
            if (at(end_bracket)) {
                next();
                list_append(params, tree_make(line(), S_EMPTY));
                continue;
            }

            list* name_list = parse_name_list();
            list* names = list_new();
            for (int i = 0; i < name_list->len; i++) {
                list_append(names, tree_make_name(line(), list_element(name_list, i)));
            }
            if (!at(end_bracket)) {
                error(line(), "bracket not properly closed");
            }
            list_append(params, tree_make_list(line(), S_COMMA, names));
            next();
        }
    }
    return params;
}

static tree* parse_def_cont(int n)
{
    int line = line();
    if (at(T_NAME)) {
        tree* names = parse_names();
        if (names->type == S_COMMA) {
            /* tuple name valdef */
            if (!at(T_VALDEF)) error(line(), "missing '='");
            next();
            tree* body = parse_com(0);
            return tree_make_binary(line, S_VALDEF, names, body);
        }

        if (at(T_VALDEF)) {
            /* single name valdef */
            next();
            tree* body = parse_com(0);
            return tree_make_binary(line, S_VALDEF, names, body);
        }

        /* function definition */
        list* params = parse_params();
        if (params->len == 0) error(line(), "no parameters");
        if (!at(T_VALDEF)) error(line(), "missing '='");
        next();
        tree* body = parse_com(0);
        for (int i = params->len-1; i >= 0; i--) {
            tree* param = list_element(params, i);
            body = tree_make_binary(line, S_LAMBDA, param, body);
        }

        return tree_make_binary(line, S_VALDEF, names, body);
    }

    if (isbracket(type())) {
        token_type end = bracket_end(type());
        next();
        tree* res = parse_def(0);
        if (!at(end)) error(line(), "unclosed bracket");
        next();
        return res;
    }

    if (at(T_REC)) {
        next();
        if (n != 0) {
            error(line(), "redundant 'rec'");
            return parse_def_cont(2);
        }
        return tree_make_unary(line, S_REC, parse_def_cont(2));
    }

    error(line(), "syntax error");

    return 0;
}

static tree* parse_def(int n)
{
    tree* res = parse_def_cont(0);
    while (at(T_AND) || at(T_WITHIN)) {
        int line = line();
        if (at(T_AND)) {
            if (!res) error(line, "definition missing before 'and'");
            if (n >= 6) return res;
            list* and_list = list_new();
            list_append(and_list, res);
            while (at(T_AND)) {
                next();
                list_append(and_list, parse_def_cont(0));
            }
            res = tree_make_list(line, S_AND, and_list);
        }
        else if (at(T_WITHIN)) {
            if (!res) error(line, "definition missing before 'within'");
            if (n >= 3) return res;
            next();
            tree* within = parse_def(0);
            res = tree_make_binary(line, S_WITHIN, res, within);
        }
    }
    return res;
}

static tree* parse_exp_cont(tree* res, int n)
{

    int line = line();
    switch (type()) {
    case T_COMMA:
        if (n > 14) return res;
        list* exp_list = list_new();
        list_append(exp_list, res);
        while (at(T_COMMA)) {
            next();
            list_append(exp_list, parse_exp(16));
        }
        res = tree_make_list(line, S_COMMA, exp_list);
        return parse_exp_cont(res, n);
    case T_AUG: {
        if (n > 16) return res;
        next();
        tree* b = parse_exp(18);
        res = tree_make_binary(line, S_AUG, res, b);
        return parse_exp_cont(res, n);
    }
    case T_COND: {
        if (n > 18) return res;
        next();
        tree* b = parse_exp(18);
        if (!at(T_BAR)) error(line(), "missing '!'");
        next();
        tree* c = parse_exp(18);
        res = tree_make_ternary(line, S_COND, res, b, c);
        return parse_exp_cont(res, n);
    }
    case T_LOGOR: {
        if (n > 20) return res;
        next();
        tree* b = parse_exp(22);
        res = tree_make_binary(line, S_LOGOR, res, b);
        return parse_exp_cont(res, n);
    }
    case T_LOGAND: {
        if (n > 22) return res;
        next();
        tree* b = parse_exp(24);
        res = tree_make_binary(line, S_LOGAND, res, b);
        return parse_exp_cont(res, n);
    }
    case T_VALDEF:
    case T_GE:
    case T_NE:
    case T_LE:
    case T_EQ:
    case T_LS:
    case T_GR: {
        if (n > 26) return res;
        tree_type op = token_to_tree_type(type());
        next();
        tree* b = parse_exp(30);
        res = tree_make_binary(line, op, res, b);
        return parse_exp_cont(res, n);
    }
    case T_PLUS:
    case T_MINUS: {
        if (n > 30) return res;
        tree_type op = token_to_tree_type(type());
        next();
        tree* b = parse_exp(32);
        res = tree_make_binary(line, op, res, b);
        return parse_exp_cont(res, n);
    }
    case T_MULT:
    case T_DIV: {
        if (n > 32) return res;
        tree_type op = token_to_tree_type(type());
        next();
        tree* b = parse_exp(34);
        res = tree_make_binary(line, op, res, b);
        return parse_exp_cont(res, n);
    }
    case T_POWER: {
        if (n > 36) return res;
        next();
        tree* b = parse_exp(34);
        res = tree_make_binary(line, S_POWER, res, b);
        return parse_exp_cont(res, n);
    }
    case T_PERCENT: {
        if (n > 36) return res;
        next();
        if (!at(T_NAME)) error(line, "'%' out of context");
        tree* b = tree_make_name(line(), cur_tok.data.string);
        next();
        tree* c = parse_exp(38);
        list* exp_list = list_new();
        list_append(exp_list, res);
        list_append(exp_list, c);
        res = tree_make_list(line, S_COMMA, exp_list);
        res = tree_make_binary(line, S_APPLY, b, res);
        return parse_exp_cont(res, n);
    }
    default:
        return res;
    }
}

static tree* parse_bracket_exp()
{
    if (!isbracket(type())) return 0;
    token_type end_bracket = bracket_end(type());
    next();
    if (at(end_bracket)) {
        /* extension: () is equivalent to nil */
        tree* res = tree_make(line(), S_NIL);
        next();
        return res;
    }
    tree* res = parse_com(0);
    if (!res) error(line(), "expression missing within brackets");
    if (!at(end_bracket)) {
        error(line(), "bracketed expression not properly closed");
    }
    next();
    return res;
}

static tree* parse_arg()
{
    tree* res;
    int line = line();
    switch (type()) {
    case T_NIL:
        next();
        return tree_make(line, S_NIL);
    case T_TRUE:
        next();
        return tree_make(line, S_TRUE);
    case T_FALSE:
        next();
        return tree_make(line, S_FALSE);
    case T_INT:
        res = tree_make_integer(line, cur_tok.data.integer);
        next();
        return res;
    case T_REAL:
        res = tree_make_real(line, cur_tok.data.real);
        next();
        return res;
    case T_STRING:
        res = tree_make_string(line, cur_tok.data.string);
        next();
        return res;
    case T_NAME:
        res = tree_make_name(line, cur_tok.data.string);
        next();
        return res;
    default:
        return parse_bracket_exp();
    }
}

static tree* apply(tree* a, int n)
{
    int line = line();
    tree* b = parse_arg();
    if (!b) return parse_exp_cont(a, n);
    a = tree_make_binary(line, S_APPLY, a, b);
    return apply(a, n);
}

static tree* parse_exp(int n)
{
    tree* res;
    int line = line();
    switch (type()) {
    case T_NOT: {
        if (n > 24) error(line(), "'not' out of context");
        next();
        res = parse_exp(26);
        res = tree_make_unary(line, S_NOT, res);
        return parse_exp_cont(res, n);
    }
    case T_PLUS:
    case T_MINUS: {
        token_type op = type();
        next();
        if (n > 30) error(line, "'+' or '-' out of context");
        res = parse_exp(32);
        res = tree_make_unary(line, op == T_PLUS?S_POS:S_NEG, res);
        return parse_exp_cont(res, n);
    }
    case T_NOSHARE:
        if (n > 36) error(line, "'$' out of context");
        next();
        res = parse_exp(38);
        res = tree_make_unary(line, S_NOSHARE, res);
        return parse_exp_cont(res, n);
    case T_NIL: {
        res = tree_make(line, S_NIL);
        next();
        return apply(res, n);
    }
    case T_TRUE: {
        res = tree_make(line, S_TRUE);
        next();
        return apply(res, n);
    }
    case T_FALSE: {
        res = tree_make(line, S_FALSE);
        next();
        return apply(res, n);
    }
    case T_INT: {
        res = tree_make_integer(line, cur_tok.data.integer);
        next();
        return apply(res, n);
    }
    case T_REAL: {
        res = tree_make_real(line, cur_tok.data.real);
        next();
        return apply(res, n);
    }
    case T_STRING: {
        res = tree_make_string(line, cur_tok.data.string);
        next();
        return apply(res, n);
    }
    case T_NAME: {
        res = tree_make_name(line, cur_tok.data.string);
        next();
        return apply(res, n);
    }
    default:
        res = parse_bracket_exp();
        if (!res) {
            if (at(T_EOF))
                error(line(), "unexpected end of source program");
            else
                error(line(), "symbol out of context");
            return 0;
        }
        return apply(res, n);
    }

    return 0;
}

tree* parse()
{
    tree* res;
    int line = line();
    switch (type()) {
    case T_DEF: {
        list* defs = list_new();
        while (at(T_DEF)) {
            next();
            list_append(defs, parse_def(0));
        }
        if (!at(T_EOF)) error(line(), "superfluous code at end of text");
        list_append(defs, tree_make(line(), S_DUMMY));
        res = tree_make(line, S_DEF);
        tree_list_size(res) = defs->len;
        res->arg.list.elements = (tree**)defs->elements;
        return res;
    }
    default:
        res = parse_com(0);
        if (!at(T_EOF)) error(line(), "superfluous code at end of text");
        return res;
    }
    return 0;
}

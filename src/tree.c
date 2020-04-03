#include <stdlib.h>
#include "tree.h"

tree* tree_make(int line, tree_type type)
{
    tree* res = malloc(sizeof(tree));
    res->type = type;
    res->line = line;
    return res;
}

tree* tree_make_list(int line, tree_type type, list* list)
{
    tree* res = tree_make(line, type);
    res->arg.list.size = list->len;
    res->arg.list.elements = (tree**)list->elements;
    return res;
}

tree* tree_make_unary(int line, tree_type type, tree* operand)
{
    tree* res = tree_make(line, type);
    res->arg.operand = operand;
    return res;
}

tree* tree_make_binary(int line, tree_type type, tree* left, tree* right)
{
    tree* res = tree_make(line, type);
    res->arg.operands.left = left;
    res->arg.operands.right = right;
    return res;
}

tree* tree_make_ternary(int line, tree_type type, tree* first, tree* second, tree* third)
{
    tree* res = tree_make(line, type);
    res->arg.list.size = 3;
    res->arg.list.elements = malloc(3*sizeof(tree*));
    res->arg.list.elements[0] = first;
    res->arg.list.elements[1] = second;
    res->arg.list.elements[2] = third;
    return res;
}

tree* tree_make_real(int line, REAL real)
{
    tree* res = tree_make(line, S_REAL);
    res->arg.real = real;
    return res;
}

tree* tree_make_integer(int line, INTEGER integer)
{
    tree* res = tree_make(line, S_INT);
    res->arg.integer = integer;
    return res;
}

tree* tree_make_name(int line, char* name)
{
    tree* res = tree_make(line, S_NAME);
    res->arg.string = name;
    return res;
}

tree* tree_make_string(int line, char* string)
{
    tree* res = tree_make(line, S_STRING);
    res->arg.string = string;
    return res;
}

static void fin(FILE* file, int indent)
{
    for (int i = 0; i < indent; i++) fprintf(file, "  ");
}

static void fln(FILE* file, int indent, char* string)
{
    fin(file, indent);
    fprintf(file, "%s\n", string);
}

void print_tree(FILE* f, tree* t, int i)
{
    switch (t->type) {
    case S_ASS:
        fln(f, i, "ASS");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_AND:
        fln(f, i, "AND");
        for (int n = 0; n < tree_list_size(t); n++) {
            print_tree(f, tree_list_element(t, n), i+1);
        }
        break;
    case S_APPLY:
        fln(f, i, "APPLY");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_AUG:
        fln(f, i, "AUG");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_COLON:
        fln(f, i, "COLON");
        print_tree(f, tree_list_element(t, 0), i+1);
        print_tree(f, tree_list_element(t, 1), i+1);
        break;
    case S_COMMA:
        fln(f, i, "COMMA");
        for (int n = 0; n < tree_list_size(t); n++) {
            print_tree(f, tree_list_element(t, n), i+1);
        }
        break;
    case S_COND:
        fln(f, i, "COND");
        print_tree(f, tree_list_element(t, 0), i+1);
        print_tree(f, tree_list_element(t, 1), i+1);
        print_tree(f, tree_list_element(t, 2), i+1);
        break;
    case S_DEF:
        fln(f, i, "DEF");
        for (int n = 0; n < tree_list_size(t); n++) {
            print_tree(f, tree_list_element(t, n), i+1);
        }
        break;
    case S_DIV:
        fln(f, i, "DIV");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_DUMMY:
        fln(f, i, "DUMMY");
        break;
    case S_EMPTY:
        fln(f, i, "EMPTY");
        break;
    case S_EQ:
        fln(f, i, "EQ");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_FALSE:
        fln(f, i, "FALSE");
        break;
    case S_GE:
        fln(f, i, "GE");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_GOTO:
        fln(f, i, "GOTO");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_GR:
        fln(f, i, "GR");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_INT:
        fln(f, i, "INT");
        fin(f, i+1);
        fprintf(f, "%ld\n", tree_integer(t));
        break;
    case S_LAMBDA:
        fln(f, i, "LAMBDA");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_LE:
        fln(f, i, "LE");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_LET:
        fln(f, i, "LET");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_LOGAND:
        fln(f, i, "LOGAND");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_LOGOR:
        fln(f, i, "LOGOR");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_LS:
        fln(f, i, "LS");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_MINUS:
        fln(f, i, "MINUS");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_MULT:
        fln(f, i, "MULT");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_NAME:
        fln(f, i, "NAME");
        fln(f, i+1, tree_string(t));
        break;
    case S_NE:
        fln(f, i, "NE");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_NEG:
        fln(f, i, "NEG");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_NIL:
        fln(f, i, "NIL");
        break;
    case S_NOSHARE:
        fln(f, i, "NOSHARE");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_NOT:
        fln(f, i, "NOT");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_PLUS:
        fln(f, i, "PLUS");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_POS:
        fln(f, i, "POS");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_POWER:
        fln(f, i, "POWER");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_REAL:
        fln(f, i, "REAL");
        fin(f, i+1);
        fprintf(f, "%f\n", tree_real(t));
        break;
    case S_REC:
        fln(f, i, "REC");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_RES:
        fln(f, i, "RES");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_SEQ:
        fln(f, i, "SEQ");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_STRING:
        fln(f, i, "STRING");
        fin(f, i+1);
        fprintf(f, "%s\n", tree_string(t));
        break;
    case S_TRUE:
        fln(f, i, "TRUE");
        break;
    case S_VALDEF:
        fln(f, i, "VALDEF");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_VALOF:
        fln(f, i, "VALOF");
        print_tree(f, tree_operand(t), i+1);
        break;
    case S_WHILE:
        fln(f, i, "WHILE");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    case S_WITHIN:
        fln(f, i, "WITHIN");
        print_tree(f, tree_left(t), i+1);
        print_tree(f, tree_right(t), i+1);
        break;
    }
}

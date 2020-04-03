#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "scanner.h"

static char *filename;
static FILE* input;
static int ch;
static int line;
static char* buf;
static int buf_len;
static int buf_max = 1024;

typedef struct {
    char* name;
    token_type type;
} name_entry;

static int name_table_max = 100;
static int name_table_size = 0;
static name_entry** name_table;

static int name_entry_compare(const void* e1, const void* e2)
{
    const name_entry* const *ne1 = e1;
    const name_entry* const *ne2 = e2;
    return strcmp((*ne1)->name, (*ne2)->name);
}

static void kw(char* name, token_type type)
{
    name_entry* entry = malloc(sizeof(name_entry));
    entry->name = name;
    entry->type = type;
    name_table[name_table_size] = entry;
    name_table_size++;
}

static void ensure_buf_size(int size)
{
    if (size > buf_max) {
        buf_max *= 2;
        buf = realloc(buf, (buf_max+1)*sizeof(char));
    }
}

void init_scanner()
{
    line = 1;
    buf = malloc((buf_max+1)*sizeof(char));
    buf_len = 0;

    name_table_size = 0;
    name_table = malloc(name_table_max*sizeof(name_entry*));

    /* register keywords */
    kw("J",      T_JJ);
    kw("and",    T_AND);
    kw("aug",    T_AUG);
    kw("def",    T_DEF);
    kw("do",     T_DO);
    kw("dummy",  T_DUMMY);
    kw("eq",     T_EQ);
    kw("false",  T_FALSE);
    kw("fn",     T_LAMBDA);
    kw("ge",     T_GE);
    kw("goto",   T_GOTO);
    kw("gr",     T_GR);
    kw("if",     T_IF);
    kw("ifnot",  T_IFNOT);
    kw("ifso",   T_IFSO);
    kw("in",     T_IN);
    kw("jj",     T_JJ);
    kw("le",     T_LE);
    kw("let",    T_LET);
    kw("ll",     T_LAMBDA);
    kw("ls",     T_LS);
    kw("ne",     T_NE);
    kw("nil",    T_NIL);
    kw("not",    T_NOT);
    kw("or",     T_LOGOR);
    kw("rec",    T_REC);
    kw("res",    T_RES);
    kw("test",   T_TEST);
    kw("true",   T_TRUE);
    kw("val",    T_VALOF);
    kw("valof",  T_VALOF);
    kw("where",  T_WHERE);
    kw("while",  T_WHILE);
    kw("within", T_WITHIN);

    qsort(name_table, name_table_size, sizeof(name_entry*), name_entry_compare);
}

static name_entry* lookup_name(char* name)
{
    name_entry key;
    key.name = name;
    name_entry* p = &key;
    name_entry** res = bsearch(&p, name_table, name_table_size, sizeof(name_entry*), name_entry_compare);
    if (res) return *res;

    if (name_table_size == name_table_max) {
        name_table_max *= 2;
        name_table = realloc(name_table, name_table_max*sizeof(name_entry*));
    }

    name_entry* new_entry = malloc(sizeof(name_entry));
    new_entry->name = strdup(name);
    new_entry->type = T_NAME;
    name_table[name_table_size] = new_entry;
    name_table_size++;
    qsort(name_table, name_table_size, sizeof(name_entry*), name_entry_compare);

    return new_entry;
}

void set_scanner_input(char* name, FILE* file)
{
    filename = name;
    input = file;
    ch = fgetc(input);
}

void scan_next(token* token)
{
    START:
    while (ch != EOF && isspace(ch)) {
        if (ch == '\n') {
            line++;
        }
        ch = fgetc(input);
    }

    if (ch == EOF) {
        token->type = T_EOF;
        token->line = line;
        return;
    }

    token->line = line;
    switch (ch) {
    case '/':
        ch = fgetc(input);
        if (ch == '/') {
            /* end of line comment */
            while (ch != EOF && ch != '\n') {
                ch = fgetc(input);
            }
            if (ch == '\n') {
                line++;
                ch = fgetc(input);
            }
            goto START;
        }
        else {
            token->type = T_DIV;
            return;
        }
        break;
    case '+':
        token->type = T_PLUS;
        break;
    case '-':
        ch = fgetc(input);
        if (ch == '>') {
            token->type = T_COND;
            break;
        }
        else {
            token->type = T_MINUS;
            return;
        }
    case '*':
        ch = fgetc(input);
        if (ch == '*') {
            token->type = T_POWER;
            break;
        }
        else {
            token->type = T_MULT;
            return;
        }
    case '.':
        token->type = T_DOT;
        break;
    case ':':
        ch = fgetc(input);
        if (ch == '=') {
            token->type = T_ASS;
            break;
        }
        else {
            token->type = T_COLON;
            return;
        }
    case '!':
        token->type = T_BAR;
        break;
    case ',':
        token->type = T_COMMA;
        break;
    case '(':
        token->type = T_LPAREN;
        break;
    case ')':
        token->type = T_RPAREN;
        break;
    case '[':
        token->type = T_LBRACKET;
        break;
    case ']':
        token->type = T_RBRACKET;
        break;
    case '{':
        token->type = T_LBRACE;
        break;
    case '}':
        token->type = T_RBRACE;
        break;
    case '$':
        token->type = T_NOSHARE;
        break;
    case '&':
        token->type = T_LOGAND;
        break;
    case '|':
        token->type = T_LOGOR;
        break;
    case '>':
        token->type = T_GR;
        break;
    case '<':
        token->type = T_LS;
        break;
    case '%':
        token->type = T_PERCENT;
        break;
    case ';':
        token->type = T_SEQ;
        break;
    case '=':
        token->type = T_VALDEF;
        break;
    case '\'':
        /* string literal */
        buf_len = 0;
        ch = fgetc(input);
        while (ch != '\'' && ch != EOF) {
            if (ch == '*') {
                ch = fgetc(input);
                if (ch == EOF) {
                    break;
                }
                else if (ch == 'n') {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = '\n';
                    ch = fgetc(input);
                    continue;
                }
                else if (ch == 't') {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = '\t';
                    ch = fgetc(input);
                    continue;
                }
                else if (ch == 's') {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = ' ';
                    ch = fgetc(input);
                    continue;
                }
                else if (ch == 'b') {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = '\b';
                    ch = fgetc(input);
                    continue;
                }
                else if (ch == '*') {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = '*';
                    ch = fgetc(input);
                    continue;
                }
                else if (ch == '\'') {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = '\'';
                    ch = fgetc(input);
                    continue;
                }
                else {
                    ensure_buf_size(buf_len+1);
                    buf[buf_len++] = '\n';
                    ch = fgetc(input);
                    continue;
                }
            }
            ensure_buf_size(buf_len+1);
            buf[buf_len++] = ch;
            ch = fgetc(input);
        }
        if (ch != '\'')
            error(line, "missing end of string quote");
        buf[buf_len++] = 0;
        token->type = T_STRING;
        token->data.string = strdup(buf);
        break;
    default:
        if (ch >= '0' && ch <= '9') {
            /* integer and real literal */
            long i = 0;
            while (ch >= '0' && ch <= '9') {
                i = i*10+(ch-'0');
                ch = fgetc(input);
            }
            if (ch == '.') {
                ch = fgetc(input);
                if (!(ch >= '0' && ch <= '9'))
                    error(line, "incorrect format of real literal");
                double d = i;
                double f = 0;
                double b = 0.1;
                while (ch >= '0' && ch <= '9') {
                    f += b*(ch-'0');
                    b /= 10;
                    ch = fgetc(input);
                }
                token->type = T_REAL;
                token->data.real = d+f;
            }
            else {
                token->type = T_INT;
                token->data.integer = i;
            }
            return;
        }
        else if ((ch >= 'a' && ch <= 'z')
                 || (ch >= 'A' && ch <= 'Z')
                 || ch == '_') {
            /* name or keyword */
            buf_len = 0;
            while ((ch >= 'a' && ch <= 'z')
                   || (ch >= 'A' && ch <= 'Z')
                   || (ch >= '0' && ch <= '9')
                   || ch == '_') {
                ensure_buf_size(buf_len+1);
                buf[buf_len++] = ch;
                ch = fgetc(input);
            }
            buf[buf_len++] = 0;
            name_entry* res = lookup_name(buf);
            token->type = res->type;
            token->data.string = res->name;
            return;
        }
        else {
            error(line, "illegal character");
            ch = fgetc(input);
            goto START;
        }
        break;
    }
    ch = fgetc(input);
}

char* token_name(token_type type)
{
    switch (type) {
    case T_AND:      return "AND";
    case T_ASS:      return "ASS";
    case T_AUG:      return "AUG";
    case T_BAR:      return "BAR";
    case T_COLON:    return "COLON";
    case T_COMMA:    return "COMMA";
    case T_COND:     return "COND";
    case T_DEF:      return "DEF";
    case T_DIV:      return "DIV";
    case T_DO:       return "DO";
    case T_DOT:      return "DOT";
    case T_DUMMY:    return "DUMMY";
    case T_EOF:      return "EOF";
    case T_EQ:       return "EQ";
    case T_FALSE:    return "FALSE";
    case T_GE:       return "GE";
    case T_GOTO:     return "GOTO";
    case T_GR:       return "GR";
    case T_IF:       return "IF";
    case T_IFNOT:    return "IFNOT";
    case T_IFSO:     return "IFSO";
    case T_IN:       return "IN";
    case T_INT:      return "INT";
    case T_JJ:       return "JJ";
    case T_LAMBDA:   return "LAMBDA";
    case T_LBRACE:   return "LBRACE";
    case T_LBRACKET: return "LBRACKET";
    case T_LE:       return "LE";
    case T_LET:      return "LET";
    case T_LOGAND:   return "LOGAND";
    case T_LOGOR:    return "LOGOR";
    case T_LPAREN:   return "LPAREN";
    case T_LS:       return "LS";
    case T_MINUS:    return "MINUS";
    case T_MULT:     return "MULT";
    case T_NAME:     return "NAME";
    case T_NE:       return "NE";
    case T_NIL:      return "NIL";
    case T_NOSHARE:  return "NOSHARE";
    case T_NOT:      return "NOT";
    case T_PERCENT:  return "PERCENT";
    case T_PLUS:     return "PLUS";
    case T_POWER:    return "POWER";
    case T_RBRACE:   return "RBRACE";
    case T_RBRACKET: return "RBRACKET";
    case T_REAL:     return "REAL";
    case T_REC:      return "REC";
    case T_RES:      return "RES";
    case T_RPAREN:   return "RPAREN";
    case T_SEQ:      return "SEQ";
    case T_STRING:   return "STRING";
    case T_TEST:     return "TEST";
    case T_TRUE:     return "TRUE";
    case T_VALDEF:   return "VALDEF";
    case T_VALOF:    return "VALOF";
    case T_WHERE:    return "WHERE";
    case T_WHILE:    return "WHILE";
    case T_WITHIN:   return "WITHIN";
    }
    return "*UNKNOWN*";
}

void print_token(FILE* file, token* t)
{
    char* name = token_name(t->type);
    switch (t->type) {
    case T_INT:
        fprintf(file, "%s: [%d] = %ld\n", name, t->line, t->data.integer);
        return;
    case T_REAL:
        fprintf(file, "%s: [%d] = %f\n", name, t->line, t->data.real);
        return;
    case T_NAME:
        fprintf(file, "%s: [%d] = %s\n", name, t->line, t->data.string);
        return;
    case T_STRING:
        fprintf(file, "%s: [%d] = '%s'\n", name, t->line, t->data.string);
        return;
    default:
        fprintf(file, "%s: [%d]\n", name, t->line);
        return;
    }
}

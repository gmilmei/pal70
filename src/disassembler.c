#include <string.h>
#include "disassembler.h"
#include "code.h"

char* op_string(op op)
{
    switch (op) {
    case OP_APPLY:          return "APPLY";
    case OP_AUG:            return "AUG";
    case OP_BLOCKLINK:      return "BLOCKLINK";
    case OP_DECLLABEL:      return "DECLLABEL";
    case OP_DECLNAME:       return "DECLNAME";
    case OP_DECLNAMES:      return "DECLNAMES";
    case OP_DIV:            return "DIV";
    case OP_DUMMY:          return "DUMMY";
    case OP_EQ:             return "EQ";
    case OP_EQU:            return "EQU";
    case OP_FALSE:          return "FALSE";
    case OP_FORMCLOSURE:    return "FORMCLOSURE";
    case OP_FORMLVALUE:     return "FORMLVALUE";
    case OP_FORMRVALUE:     return "FORMRVALUE";
    case OP_GE:             return "GE";
    case OP_GOTO:           return "GOTO";
    case OP_GR:             return "GR";
    case OP_INITNAME:       return "INITNAME";
    case OP_INITNAMES:      return "INITNAMES";
    case OP_JJ:             return "JJ";
    case OP_JUMP:           return "JUMP";
    case OP_JUMPF:          return "JUMPF";
    case OP_LABEL:          return "LABEL";
    case OP_LE:             return "LE";
    case OP_LOADE:          return "LOADE";
    case OP_LOADF:          return "LOADF";
    case OP_LOADGUESS:      return "LOADGUESS";
    case OP_LOADL:          return "LOADL";
    case OP_LOADN:          return "LOADN";
    case OP_LOADR:          return "LOADR";
    case OP_LOADS:          return "LOADS";
    case OP_LOGAND:         return "LOGAND";
    case OP_LOGOR:          return "LOGOR";
    case OP_LOSE1:          return "LOSE1";
    case OP_LS:             return "LS";
    case OP_MEMBERS:        return "MEMBERS";
    case OP_MINUS:          return "MINUS";
    case OP_MULT:           return "MULT";
    case OP_NE:             return "NEW";
    case OP_NEG:            return "NEG";
    case OP_NIL:            return "NIL";
    case OP_NOT:            return "NOT";
    case OP_PARAM:          return "PARAM";
    case OP_PLUS:           return "PLUS";
    case OP_POS:            return "POS";
    case OP_POWER:          return "POWER";
    case OP_RES:            return "RES";
    case OP_RESLINK:        return "RESLINK";
    case OP_RESTOREE1:      return "RESTOREE1";
    case OP_RETURN:         return "RETURN";
    case OP_SAVE:           return "SAVE";
    case OP_SETLABES:       return "SETLABES";
    case OP_SETUP:          return "SETUP";
    case OP_TESTEMPTY:      return "TESTEMPTY";
    case OP_TRUE:           return "TRUE";
    case OP_TUPLE:          return "TUPLE";
    case OP_UPDATE:         return "UPDATE";
    default:                return "*UNKNOWN*";
    }

}

static void out_op(FILE* out, int addr, BYTE op)
{
    fprintf(out, "%04x: [%02x] %s", addr, op, op_string(op));
}

static void out_ln(FILE* out)
{
    fprintf(out, "\n");
}

static void out_integer(FILE* out, long i)
{
    fprintf(out, " %ld", i);
}

static void out_real(FILE* out, double f)
{
    fprintf(out, " %f", f);
}

static void out_string(FILE* out, char* s)
{
    fprintf(out, " ");
    while (*s) {
        switch (*s) {
        case '\'':
            fprintf(out, "*'");
            break;
        case '\n':
            fprintf(out, "*n");
            break;
        default:
            fprintf(out, "%c", *s);
        }
        s++;
    }
}

void disassemble(FILE* out, BYTE* code, int code_len, char** files)
{
    int n = 0;
    while (n < code_len) {
        op op = code[n];
        int addr = n++;
        int line = decode_int(&code[n]);
        n += 3;
        char* file_name = files[line>>24];
        line = line&0xFFFFFF;
        if (0) fprintf(out, "%s: %d\n", file_name, line);
        switch (op) {
        case OP_SETLABES:
        case OP_PARAM:
        case OP_TUPLE:
        case OP_UPDATE:
        case OP_SETUP: {
            n++;
            int i = decode_int(&code[n]);
            n += 4;
            out_op(out, addr, op);
            out_integer(out, i);
            out_ln(out);
            break;
        }
        case OP_LOADN: {
            n++;
            INTEGER i = decode_integer(&code[n]);
            n += 8;
            out_op(out, addr, op);
            out_integer(out, i);
            out_ln(out);
            break;
        }
        case OP_LOADF: {
            n++;
            REAL real = decode_real(&code[n]);
            n += 8;
            out_op(out, addr, op);
            out_real(out, real);
            out_ln(out);
            break;
        }
        case OP_DECLNAME:
        case OP_DECLLABEL:
        case OP_INITNAME:
        case OP_LOADS:
        case OP_LOADL:
        case OP_LOADR: {
            n++;
            int len = decode_int(&code[n]);
            n += 4;
            char s[len+1];
            decode_string(&code[n], s, len);
            n += len;
            out_op(out, addr, op);
            out_string(out, s);
            out_ln(out);
            break;
        }
        case OP_EQU: {
            n++;
            int L = decode_int(&code[n]);
            n += 4;
            int N = decode_int(&code[n]);
            n += 4;
            out_op(out, addr, op);
            out_integer(out, L);
            out_integer(out, N);
            out_ln(out);
            break;
        }
        case OP_LABEL: {
            n++;
            int i = decode_int(&code[n]);
            n += 4;
            fprintf(out, "%04x: [%02x] L%d:\n", addr, op, i);
            break;
        }
        case OP_DECLNAMES: {
            n++;
            int len = decode_int(&code[n]);
            n += 4;
            out_op(out, addr, op);
            for (int i = 0; i < len; i++) {
                int slen = decode_int(&code[n]);
                n += 4;
                char s[slen+1];
                decode_string(&code[n], s, slen);
                n += slen;
                out_string(out, s);
            }
            out_ln(out);
            break;
        }
        case OP_MEMBERS: {
            n++;
            int i = decode_int(&code[n]);
            n += 4;
            out_op(out, addr, op);
            out_integer(out, i);
            out_ln(out);
            break;
        }
        default:
            /* operations with no arguments */
            n++;
            out_op(out, addr, op); out_ln(out);
            break;
        }
    }
}

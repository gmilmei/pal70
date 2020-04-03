#include <stdarg.h>
#include "error.h"
#include "builtins.h"

int err_count = 0;

static char* filename;
static FILE* err;
static int max_err_count = 5;
static char* err_file;
static int err_line;

void init_error(char* f, FILE* e)
{
    filename = f;
    err = e;
}

void error(int line, char* msg)
{
    if (err_count < max_err_count) {
        fprintf(err, "%s:%d: %s\n", filename, line&0xFFFFFF, msg);
        err_count++;
    }
}

void set_error_location(char* file, int line)
{
    err_file = file;
    err_line = line;
}

void runtime_error(char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    fprintf(err, "%s:%d:runtime error: ", err_file, err_line);
    while (*format) {
        if (*format == '%') {
            format++;
            if (!*format) {
                break;
            }
            else if (*format == 's') {
                char* s = va_arg(argp, char*);
                fprintf(err, "%s", s);
            }
            else if (*format == 'd') {
                INTEGER i = va_arg(argp, INTEGER);
                fprintf(err, "%ld", i);
            }
            else if (*format == 'v') {
                value* v = va_arg(argp, value*);
                fprintval(err, v, 0, 1);
            }
            else {
                fputc(*format, err);
            }
        }
        else {
            fputc(*format, err);
        }
        format++;
    }
    fprintf(err, "\n");
    va_end(argp);
}

void lookup_error(char* name)
{
    runtime_error("unknown name '%s'", name);
}

void apply_error(char* op, value* val1, value* val2)
{
    if (val2)
        runtime_error("'%s' applied to %v and %v", op, val1, val2);
    else
        runtime_error("'%s' applied to %v", op, val1);
}

#include <stdlib.h>
#include <string.h>
#include "strings.h"

typedef struct {
    int size;
    int max;
    char** strings;
} string_table;

static string_table* ST;

void init_strings()
{
    ST = malloc(sizeof(string_table));
    ST->size = 0;
    ST->max = 512;
    ST->strings = calloc(ST->max, sizeof(char*));
}

int string_to_ref(char* string)
{
    int size = ST->size;
    for (int i = 0; i < size; i++) {
        if (strcmp(string, ST->strings[i]) == 0) {
            return i;
        }
    }
    size++;
    if (size > ST->max) {
        ST->max *= 2;
        ST->strings = realloc(ST->strings, ST->max*sizeof(char*));
    }
    ST->size = size;
    ST->strings[size-1] = strdup(string);
    return size-1;
}

int string_to_ref_if_exists(char* string)
{
    int size = ST->size;
    for (int i = 0; i < size; i++) {
        if (strcmp(string, ST->strings[i]) == 0) {
            return i;
        }
    }
    return -1;
}

char* ref_to_string(int ref)
{
    return ST->strings[ref];
}

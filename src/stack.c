#include "stack.h"

void print_stack(FILE* file, stack* S)
{
    int i = 0;
    while (S) {
        fprintf(file, "%02d: ", i);
        if (S && S->value) print_value(file, S->value);
        fprintf(file, "\n");
        i++;
        S = S->next;
    }
}

#include <stdlib.h>
#include "list.h"

list* list_new()
{
    list* list = malloc(sizeof(list));
    list->len = 0;
    list->max = 16;
    list->elements = malloc(list->max*sizeof(void*));
    return list;
}

void list_append(list* list, void* element)
{
    if (list->len == list->max) {
        list->max *= 2;
        list->elements = realloc(list->elements, list->max*sizeof(void*));
    }
    list->elements[list->len] = element;
    list->len++;
}

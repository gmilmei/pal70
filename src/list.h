#ifndef LIST_H
#define LIST_H

typedef struct {
    int len;
    int max;
    void** elements;
} list;

#define list_element(_L, _N) ((_L)->elements[_N])

/*
 * Creates a new empty list.
 */
list* list_new();

/*
 * Appends element to the end of list.
 */
void list_append(list* list, void* element);

#endif

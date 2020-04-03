#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include "tree.h"
#include "config.h"

/*
 * Must be called before invoking translate.
 */
void init_translator();

/*
 * Translates the tree and returns the byte array
 * containing, its length in code_len.
 */
BYTE* translate(tree* t, int* code_len);

/**
 * Translates a list of trees.
 */
BYTE* translate_list(list* tree_list, int* code_len);

#endif

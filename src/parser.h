#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "tree.h"

void init_parser(char* filename, FILE* file, int filenr);

tree* parse();

#endif

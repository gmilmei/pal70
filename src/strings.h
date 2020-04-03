#ifndef STRINGS_H
#define STRINGS_H

void init_strings();

int string_to_ref(char* string);

int string_to_ref_if_exists(char* string);

char* ref_to_string(int ref);

#endif

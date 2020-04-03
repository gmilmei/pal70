#include <stdlib.h>
#include <string.h>
#include "code.h"
#include "config.h"

typedef union {
    REAL real;
    INTEGER integer;
    int i;
    BYTE bytes[8];
} encode_buf;

void encode_real(REAL real, BYTE* bytes)
{
    encode_buf* e =  (encode_buf*)bytes;
    e->real = real;
    if (SYSTEM_LITTLE_ENDIAN) {
        for (int i = 0; i < 4; i++) {
            BYTE c = e->bytes[i];
            e->bytes[i] = e->bytes[7-i];
            e->bytes[7-i] = c;
        }
    }
}

REAL decode_real(BYTE* bytes)
{
    encode_buf e;
    if (SYSTEM_LITTLE_ENDIAN) {
        for (int i = 0; i < 8; i++) {
            e.bytes[i] = bytes[7-i];
        }
    }
    return e.real;
}

void encode_integer(INTEGER integer, BYTE* bytes)
{
    encode_buf* e =  (encode_buf*)bytes;
    e->integer = integer;
    if (SYSTEM_LITTLE_ENDIAN) {
        for (int i = 0; i < 4; i++) {
            BYTE c = e->bytes[i];
            e->bytes[i] = e->bytes[7-i];
            e->bytes[7-i] = c;
        }
    }
}

INTEGER decode_integer(BYTE* bytes)
{
    encode_buf e;
    if (SYSTEM_LITTLE_ENDIAN) {
        for (int i = 0; i < 8; i++) {
            e.bytes[i] = bytes[7-i];
        }
    }
    return e.integer;
}

void encode_int(int i, BYTE* bytes)
{
    encode_buf* e =  (encode_buf*)bytes;
    e->i = i;
    if (SYSTEM_LITTLE_ENDIAN) {
        for (int i = 0; i < 2; i++) {
            BYTE c = e->bytes[i];
            e->bytes[i] = e->bytes[3-i];
            e->bytes[3-i] = c;
        }
    }
}

int decode_int(BYTE* bytes)
{
    encode_buf e;
    if (SYSTEM_LITTLE_ENDIAN) {
        for (int i = 0; i < 4; i++) {
            e.bytes[i] = bytes[3-i];
        }
    }
    return e.i;
}

void decode_string(BYTE* bytes, char* string, int len)
{
    for (int i = 0; i < len; i++) string[i] = bytes[i];
    string[len] = 0;
}

void write_code(FILE* file, BYTE* bytes, int len, char** files, int files_len)
{
    BYTE int_buf[4];

    /* write header */
    fputc(0xF0, file);
    fprintf(file, "%s", "POCODE70");
    fputc(0x00, file);

    /* write number of source file names */
    encode_int(files_len, int_buf);
    fwrite(int_buf, 1, 4, file);

    /* write each source file name */
    for (int i = 0; i < files_len; i++) {
        /* write length of file name */
        int nlen = strlen(files[i]);
        encode_int(nlen, int_buf);
        fwrite(int_buf, 1, 4, file);
        /* write file name */
        fprintf(file, "%s", files[i]);
    }

    /* write length of code */
    encode_int(len, int_buf);
    fwrite(int_buf, 1, 4, file);

    /* write code */
    fwrite(bytes, 1, len, file);
    fflush(file);
}

BYTE* read_code(FILE* file, int* len, char*** files, int* files_len)
{
    BYTE buf[9];

    /* read header */
    int ch = fgetc(file);
    BYTE* bytes = 0;
    if (ch != 0xF0) return 0;
    int n = fread(buf, 1, 9, file);
    if (n != 9) return 0;

    /* read number of source file names */
    n = fread(buf, 1, 4, file);
    if (n != 4) return 0;
    int flen = decode_int(buf);
    *files_len = flen;

    /* read source file names */
    char** file_names = malloc(flen*sizeof(char*));
    *files = file_names;
    for (int i = 0; i < flen; i++) {
        /* read length of file name */
        n = fread(buf, 1, 4, file);
        if (n != 4) return 0;
        int nlen = decode_int(buf);
        /* read file name */
        char* file_name = calloc((nlen+1), sizeof(char));
        n = fread(file_name, sizeof(char), nlen, file);
        if (n != nlen) return 0;
        file_names[i] = file_name;
    }

    /* read length of code */
    n = fread(buf, 1, 4, file);
    if (n != 4) return 0;
    *len = decode_int(buf);
    if (*len < 0) return 0;

    /* read code */
    bytes = malloc(*len*sizeof(BYTE));
    n = fread(bytes, 1, *len, file);
    if (n != *len) return 0;

    return bytes;
}

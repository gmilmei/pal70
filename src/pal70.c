#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "error.h"
#include "parser.h"
#include "translator.h"
#include "disassembler.h"
#include "interpreter.h"
#include "code.h"

static int verbose = 0;

static int disass(char* prg, char* file_name)
{
    if (!file_name) {
        fprintf(stderr, "%s: missing file name\n", prg);
        return 1;
    }

    FILE* code_in = fopen(file_name, "r");
    if (!code_in) {
        perror(prg);
        return 1;
    }

    int code_len;
    char** file_names;
    int file_names_len;
    BYTE* code = read_code(code_in, &code_len, &file_names, &file_names_len);
    if (!code) {
        fprintf(stderr, "%s: error reading %s\n", prg, file_name);
        return 1;
    }
    fclose(code_in);

    if (verbose) fprintf(stdout, "Disassembly of %s\n", file_name);
    disassemble(stdout, code, code_len, file_names);

    return 0;
}

static int compile(char* prg, char** file_names, char* output_file_name)
{
    if (!output_file_name) output_file_name = "pocode.out";

    list* tree_list = list_new();
    int i = 0;
    while (file_names[i]) {
        char* file_name = file_names[i];
        if (verbose) fprintf(stdout, "Parsing %s\n", file_name);

        FILE* source_in = fopen(file_name, "r");
        if (!source_in) {
            perror(prg);
            return 1;
        }

        init_error(file_name, stderr);
        init_parser(file_name, source_in, i);
        tree* tree = parse();
        fclose(source_in);

        if (err_count != 0) return 1;

        list_append(tree_list, tree);
        i++;
    }

    if (verbose) fprintf(stdout, "Translating\n");
    init_translator();
    int code_len;
    BYTE* code = translate_list(tree_list, &code_len);

    if (err_count != 0) return 1;

    if (verbose) fprintf(stdout, "Writing code to %s\n", output_file_name);
    FILE* code_out = fopen(output_file_name, "w");
    if (!code_out) {
        perror(prg);
        return 1;
    }
    write_code(code_out, code, code_len, file_names, i);
    fclose(code_out);

    return 0;
}

static int run(char* prg, char* file_name)
{
    FILE* code_in = fopen(file_name, "r");
    if (!code_in) {
        perror(prg);
        return 1;
    }

    char** file_names;
    int file_names_len;
    int code_len;
    BYTE* code = read_code(code_in, &code_len, &file_names, &file_names_len);
    if (!code) {
        fprintf(stderr, "%s: error reading %s\n", prg, file_name);
        return 1;
    }
    fclose(code_in);

    init_error(file_name, stderr);
    init_interpreter(code, code_len, file_names);
    if (verbose) fprintf(stdout, "Executing %s\n", file_name);
    execute();
    if (verbose) fprintf(stdout, "Terminated\n");

    return 0;
}

static void print_usage(FILE* file, char* prg)
{
    fprintf(file, "Usage: %s [-c] [-h] [-d] [-v] [-o FILE] FILE...\n", prg);
}

int main(int argc, char* argv[])
{
    int opt;
    int do_disass = 0;
    int do_compile = 0;
    char* file_name = 0;
    char* output_file_name = 0;
    char* prg = argv[0];

    while ((opt = getopt(argc, argv, "vhcdo:")) != -1) {
        switch (opt) {
        case 'd':
            do_disass = 1;
            break;
        case 'c':
            do_compile = 1;
            break;
        case 'o':
            output_file_name = strdup(optarg);
            break;
        case 'h':
            print_usage(stdout, prg);
            return 0;
        case 'v':
            verbose = 1;
            break;
        default:
            print_usage(stderr, prg);
            return 1;
        }
    }

    file_name = argv[optind];

    if (do_disass) {
        return disass(prg, file_name);
    }

    if (do_compile) {
        int nrfiles = argc-optind;
        if (nrfiles == 0) {
            fprintf(stderr, "%s: missing file name\n", prg);
            exit(1);
        }
        char** file_names = calloc(nrfiles+1, sizeof(char*));
        int i = 0;
        while (optind < argc) {
            file_names[i] = argv[optind];
            i++;
            optind++;
        }

        return compile(prg, file_names, output_file_name);
    }

    if (file_name) {
        return run(prg, file_name);
    }

    print_usage(stderr, prg);
    return 1;
}

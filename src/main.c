/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "a_string_slice.h"
#include "error.h"
#include "lexer.h"
#include "lexer_types.h"
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <threads.h>

#include "a_string.h"
#include "a_vector.h"
#include "common.h"

#define _UTIL_H_IMPLEMENTATION
#include "util.h"

typedef struct {
    a_string in_path;
    a_string out_path;
    bool has_in_path;
    bool has_out_path;
    bool debug;
    bool no_compile;
    bool help;
} Args;

static const struct option LONG_OPTS[] = {
    {"out-path", required_argument, 0, 'o'},
    {"debug", required_argument, 0, 'd'},
    {"no-compile", required_argument, 0, 'N'},
    {"help", required_argument, 0, 'h'},
    {0},
};

void help(void);
bool parse_args(int argc, char** argv);
void init(int argc, char** argv);
void compile(void);
void deinit(void);

static Args args;

void help(void) {
    puts("  --out-path, -o: specify output path (default: out.qbe)");
    puts("  --debug, -d: print extra debugging info");
    puts("  --no-compile, -N: don't actually compile anything");
}

bool parse_args(int argc, char** argv) {
    args = (Args){0};

    int c = 0;
    while ((c = getopt_long(argc, argv, "o:dhN", LONG_OPTS, NULL)) != -1) {
        switch (c) {
            case 'o': {
                args.out_path = astr(optarg);
                args.has_out_path = true;
            } break;
            case 'd': {
                args.debug = true;
            } break;
            case 'N': {
                args.no_compile = true;
            } break;
            case 'h': {
                args.help = true;
            } break;
            case '?': {
                help();
                return false;
            } break;
        }
    }

    if (optind < argc) {
        args.in_path = astr(argv[optind]);
        if (args.in_path.len == 0)
            panic("no input file provided");
        args.has_in_path = true;
    }

    return true;
}

void init(int argc, char** argv) {
    args = (Args){0};

    parse_args(argc, argv);
}

static a_string file_content;
static a_string file_name;

void compile(void) {
    if (!args.has_in_path) {
        file_name = astr("(stdin)");
        file_content = as_read_line(stdin);
        if (!as_valid(&file_content)) {
            as_free(&file_name);
            panic("could not read line from stdin");
        }
    } else {
        file_name = astr(args.in_path.data);
        file_content = as_read_file(args.in_path.data);
        if (errno == ENOENT)
            panic("file \"%s\" not found", args.in_path.data);
    }

    a_string_slice src_view = ass_from_astr(file_content);
    CBCLexer l = cbc_lexer_new(src_view);
    CBCToken* tokens = NULL;
    usize len = cbc_lexer_tokenize(&l, &tokens, ass_from_astr(file_name));

    if (l.error.kind) {
        eprintf("error occurred while tokenizing\n");
        free(tokens);
    }

    for (usize i = 0; i < len; i++) {
        a_string_slice slc =
            cbc_token_to_string_slice_full(&tokens[i], src_view);
        printf("%.*s\n", as_fmt(slc));
    }
    free(tokens);

    return;
}

void deinit(void) {
    as_free(&file_content);
    as_free(&file_name);
}

i32 main(i32 argc, char** argv) {
    init(argc, argv);

    if (args.help) {
        help();
    } else {
        compile();
    }

    deinit();
    return 0;
}

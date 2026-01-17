/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <threads.h>

#include "a_string.h"
#include "a_vector.h"
#include "ast.h"
#include "ast_printer.h"
#include "common.h"
#include "compiler.h"
#include "lexer.h"
#include "parser/parser.h"

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
bool compile(void);
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
            fatal("no input file provided");
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
static Lexer l;
static Tokens toks;
static Parser ps;
static CB_Program prog;
static AstPrinter printer;
static Compiler comp;

bool compile(void) {
    if (!args.has_in_path) {
        file_name = astr("(stdin)");
        file_content = as_new();
        if (!as_read_line(&file_content, stdin))
            panic("could not read line from stdin");
    } else {
        file_name = astr(args.in_path.data);
        file_content = as_read_file(args.in_path.data);
        if (errno == ENOENT)
            panic("file \"%s\" not found", args.in_path.data);
    }

    l = lx_new(file_content.data, file_content.len);

    toks = (Tokens){0};
    if (!lx_tokenize(&l, &toks))
        return false;

    if (args.debug) {
        printf("\x1b[2m=== TOKENS ===\n");
        for (usize i = 0; i < toks.len; i++) {
            token_print_long(&toks.data[i]);
        }
        printf("==============\x1b[0m\n");
    }

    ps = ps_new(toks.data, toks.len, as_dupe(&file_name));

    if (!ps_program(&ps, &prog)) {
        eprintf("error\n");
        return false;
    }

    if (args.debug) {
        printer = ap_new();
        ap_visit_program(&printer, &prog);
        putchar('\n');
    }

    const char* out_name = args.has_out_path ? args.out_path.data : "out.qbe";
    if (!cm_new_with_file_writer(out_name, &comp))
        panic("could not open %s", out_name);

    cm_program(&comp, &prog);

    eprintf("Compiled %.*s to %s\n", as_fmt(args.in_path), out_name);

    return true;
}

void deinit(void) {
    cm_free(&comp);
    cb_program_free(&prog);
    ps_free(&ps);
    for (usize i = 0; i < toks.len; i++) {
        token_free(&toks.data[i]);
    }
    av_free(&toks);
    lx_free(&l);
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

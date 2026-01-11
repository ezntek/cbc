/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <stdio.h>
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <errno.h>
#include <string.h> // used by macro

#include "a_string.h"
#include "a_vector.h"
#include "common.h"
#include "lexer.h"
#include "lexertypes.h"

// #include "tests/ast_printer.c"

i32 main(i32 argc, char** argv) {
    argv++;
    argc--;

    a_string s = {0};
    if (argc == 0) {
        s = as_new();
        if (!as_read_line(&s, stdin))
            panic("could not read line from stdin");
    } else {
        s = as_read_file(argv[0]);
        if (errno == ENOENT)
            panic("file \"%s\" not found", argv[0]);
    }

    a_string filename = {0};
    if (argc == 0) {
        filename = astr("(stdin)");
    } else {
        filename = astr(argv[0]);
    }

    Lexer l = lx_new(s.data, s.len);
    Tokens toks = lx_tokenize(&l);

    for (usize i = 0; i < toks.len; i++) {
        token_free(&toks.data[i]);
    }
    av_free(&toks);
    lx_free(&l);
    as_free(&s);
    as_free(&filename);
    return 0;
}

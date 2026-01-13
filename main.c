/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ast_printer.h"
#define _POSIX_C_SOURCE 200809L

#include <errno.h>

#include "a_string.h"
#include "a_vector.h"
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"

#define _UTIL_H_IMPLEMENTATION
#include "util.h"

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

    if (!toks.len)
        goto end;

    printf("\x1b[2m=== TOKENS ===\n");
    for (usize i = 0; i < toks.len; i++) {
        token_print_long(&toks.data[i]);
    }
    printf("==============\x1b[0m\n");

    Parser ps = ps_new(toks.data, toks.len, as_dupe(&filename));

    if (!ps_expr(&ps)) {
        eprintf("error\n");
        goto end;
    }

    AstPrinter ap = ap_new();
    ap_visit_expr(&ap, &ps.expr);
    putchar('\n');

    cb_expr_free(&ps.expr);
end:
    ps_free(&ps);
    for (usize i = 0; i < toks.len; i++) {
        token_free(&toks.data[i]);
    }
    av_free(&toks);
    lx_free(&l);
    as_free(&s);
    as_free(&filename);
    return 0;
}

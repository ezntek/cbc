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

#include "../lexer.h"
#include "parser.h"
#include "parser_internal.h"

AV_DECL(CB_Expr, Exprs)

void ps_skip_stmt(Parser* ps) {
    while (!ps->eof && ps_check(ps, TOK_NEWLINE))
        ps_consume(ps);
}

static bool ps_output_stmt(Parser* ps) {
    MaybeToken mt = ps_check_and_consume(ps, TOK_OUTPUT);
    Token* begin = NULL;
    Exprs exprs = {0};

    if (!mt.have) {
        mt = ps_check_and_consume(ps, TOK_PRINT);
        if (!mt.have)
            return false;
    }
    begin = mt.data;

    if (ps_check(ps, TOK_NEWLINE))
        goto end;
    // call above it sets eof
    else if (ps->eof)
        goto end;

    if (!ps_expr(ps)) {
        ps_diag(ps, "could not parse expression after %s",
                token_kind_string(begin->kind));
        goto fail;
    } else {
        av_append(&exprs, ps->expr);
    }

    while (!ps_check(ps, TOK_NEWLINE) && !ps->eof) {
        if (!ps_consume_and_expect(ps, TOK_COMMA).have) {
            ps_skip_stmt(ps);
            goto fail;
        }

        if (!ps_expr(ps)) {
            ps_diag(ps, "could not parse expression");
            goto fail;
        } else {
            av_append(&exprs, ps->expr);
        }
    }

    // move Exprs over to OutputStmt
end:
    CB_OutputStmt output_stmt = cb_output_stmt_new(exprs.data, exprs.len);
    ps->stmt = cb_stmt_new_output(begin->pos, output_stmt);
    return true;
fail:
    av_free(&exprs);
    return false;
}

bool ps_stmt(Parser* ps) {
    if (ps_output_stmt(ps))
        return true;

    if (!ps->eof)
        ps_diag(ps, "unexpected token %s",
                token_kind_string(ps_peek(ps).data->kind));
    return false;
}

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

void ps_skip_past_newline(Parser* ps) {
    while (!ps->eof && ps_check(ps, TOK_NEWLINE))
        ps_consume(ps);
    // eat the last newline
    if (!ps->eof)
        ps_consume(ps);
}

static bool ps_output_stmt(Parser* ps) {
    Token* begin = NULL;
    Exprs exprs = {0};

    if (!ps_check(ps, TOK_OUTPUT) && !ps_check(ps, TOK_PRINT))
        return false;

    begin = ps_consume(ps);
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
        if (!ps_consume_and_expect(ps, TOK_COMMA)) {
            ps_skip_past_newline(ps);
            goto fail;
        }

        if (!ps_expr(ps)) {
            ps_diag(ps, "could not parse expression");
            goto fail;
        } else {
            av_append(&exprs, ps->expr);
        }
    }

    (void)ps_check_and_consume(ps, TOK_NEWLINE);

    // TCC fix
    CB_OutputStmt output_stmt = {0};
end:
    // move Exprs over to OutputStmt
    output_stmt = cb_output_stmt_new(exprs.data, exprs.len);
    ps->stmt = cb_stmt_new_output(begin->pos, output_stmt);
    return true;
fail:
    av_free(&exprs);
    return false;
}

static bool ps_input_stmt(Parser* ps) {
    Token* begin = NULL;

    if (!(begin = ps_check_and_consume(ps, TOK_INPUT)))
        return false;

    if (!ps_expr(ps)) {
        ps_diag_at(ps, begin->pos, "could not parse expression after INPUT");
        return false;
    }

    CB_InputStmt input_stmt = cb_input_stmt_new(ps->expr);
    ps->stmt = cb_stmt_new_input(begin->pos, input_stmt);
    return true;
}

bool ps_stmt(Parser* ps) {
    if (ps->eof) {
        ps_diag(ps, "unexpected end of file");
        return false;
    }

    if (ps_output_stmt(ps))
        return true;

    if (ps_input_stmt(ps))
        return true;

    if (ps_expr(ps)) {
        ps->stmt = cb_stmt_new_expr(ps->expr.pos, ps->expr);
        return true;
    }

    if (!ps->eof)
        ps_diag(ps, "unexpected token %s",
                token_kind_string(ps_peek(ps)->kind));

    return false;
}

AV_DECL(CB_Stmt, Stmts)

bool ps_program(Parser* ps, CB_Program* out) {
    Stmts s = {0};

    while (true) {
        if (ps_check(ps, TOK_NEWLINE))
            (void)ps_consume(ps);

        if (ps->eof)
            break;

        if (ps_stmt(ps))
            av_append(&s, ps->stmt);
        else
            ps_skip_past_newline(ps);

        if (ps->error_count > MAX_ERROR_COUNT) {
            ps_diag(ps, "too many errors, stopping here");
            goto fail;
        }
    }

    if (ps->error_reported)
        goto fail;

    *out = cb_program_new(s.data, s.len);
    return true;
fail:
    av_free(&s);
    return false;
}

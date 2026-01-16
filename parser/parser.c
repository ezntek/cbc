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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "../a_string.h"
#include "../common.h"
#include "../lexer.h"
#include "../lexer_types.h"
#include "parser.h"
#include "parser_internal.h"

MaybeToken ps_consume(Parser* ps) {
    if (ps->cur < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur++]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

MaybeToken ps_peek(Parser* ps) {
    if (ps->cur < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

MaybeToken ps_peek_next(Parser* ps) {
    if (ps->cur + 1 < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur + 1]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

MaybeToken ps_prev(Parser* ps) {
    if (ps->cur - 1 < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur - 1]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

MaybeToken ps_get(Parser* ps, usize idx) {
    if (idx < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[idx]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

MaybeToken ps_peek_and_expect(Parser* ps, TokenKind expected) {
    const char* expected_s = token_kind_string(expected);
    if_let(Token*, t, ps_peek(ps)) {
        if (t->kind == TOK_EOF) {
            ps_diag(ps, "expected token %s, but reached end of file",
                    token_kind_string(expected));
        } else if (t->kind != expected) {
            ps_diag(ps, "expected token %s, but found %s",
                    token_kind_string(t->kind), expected_s);
        } else {
            return HAVE_TOKEN(t);
        }
        return NO_TOKEN;
    }

    ps_diag(ps, "expected token %s, but got no token", expected_s);
    return NO_TOKEN;
}

bool ps_check(Parser* ps, TokenKind expected) {
    if_let(Token*, t, ps_peek(ps)) {
        return t->kind == expected;
    }
    return false;
}

MaybeToken ps_check_and_consume(Parser* ps, TokenKind expected) {
    if (ps_check(ps, expected)) {
        return ps_consume(ps);
    } else {
        return NO_TOKEN;
    }
}

MaybeToken ps_consume_and_expect(Parser* ps, TokenKind expected) {
    const char* expected_s = token_kind_string(expected);
    if_let(Token*, t, ps_consume(ps)) {
        if (t->kind != expected) {
            ps_diag(ps, "expected token \"%s\" but got \"%s\"", expected_s,
                    token_kind_string(t->kind));
        } else {
            return HAVE_TOKEN(t);
        }
        return NO_TOKEN;
    }

    ps_diag(ps, "expected token \"%s\" but reached the end of the token stream",
            expected_s);
    return NO_TOKEN;
}

Pos ps_get_pos(Parser* ps) {
    if_let(Token*, t, ps_peek(ps)) {
        return t->pos;
    }

    if (ps->tokens_len > 0)
        return ps->tokens[ps->tokens_len - 1].pos;

    return (Pos){
        .col = 1,
        .row = 1,
    };
}

void ps_diag_at(Parser* ps, Pos pos, const char* format, ...) {
    eprintf("\033[31;1merror: \033[0;1m%.*s:%u:%u: \033[0m",
            (int)ps->file_name.len, ps->file_name.data, pos.row, pos.col);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    putchar('\n');
    ps->error_reported = true;
}

void ps_diag(Parser* ps, const char* format, ...) {
    Pos pos = ps_get_pos(ps);
    // FIXME: less code duplication due to va_list
    eprintf("\033[31;1merror: \033[0;1m%.*s:%u:%u: \033[0m",
            (int)ps->file_name.len, ps->file_name.data, pos.row, pos.col);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    putchar('\n');
    ps->error_reported = true;
}

bool ps_bump_error_count(Parser* ps) {
    return ++ps->cur > MAX_ERROR_COUNT;
}

void ps_diag_expected(Parser* ps, const char* thing) {
    if_let(Token*, tok, ps_peek(ps)) {
        ps_diag(ps, "expected %s, but found token \"%s\"",
                token_kind_string(tok->kind));
        return;
    }

    ps_diag(ps, "expected %s, but found no token", thing);
}

Parser ps_new(Token* tokens, usize tokens_len, a_string file_name) {
    if (tokens_len < 1)
        panic("invalid token length (missing EOF token)");

    return (Parser){
        .tokens = tokens, .tokens_len = tokens_len - 1, .file_name = file_name};
}

void ps_free(Parser* ps) {
    as_free(&ps->file_name);
}

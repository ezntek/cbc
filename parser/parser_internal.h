/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _PARSER_INTERNAL_H
#define _PARSER_INTERNAL_H

#include "../lexer_types.h"
#include "parser.h"

typedef struct {
    Token* data;
    bool have;
} MaybeToken;

#define HAVE_TOKEN(t)                                                          \
    (MaybeToken) {                                                             \
        .data = (t), .have = true                                              \
    }

#define NO_TOKEN (MaybeToken){0}

// utility functions
MaybeToken ps_peek(Parser* ps);
MaybeToken ps_consume(Parser* ps);
MaybeToken ps_prev(Parser* ps);
MaybeToken ps_peek_next(Parser* ps);
MaybeToken ps_get(Parser* ps, usize idx);
MaybeToken ps_peek_and_expect(Parser* ps, TokenKind expected);
bool ps_check(Parser* ps, TokenKind expected);
MaybeToken ps_check_and_consume(Parser* ps, TokenKind expected);
MaybeToken ps_consume_and_expect(Parser* ps, TokenKind expected);
Pos ps_get_pos(Parser* ps);
bool ps_bump_error_count(Parser* ps);
void ps_diag(Parser* ps, const char* format, ...);
void ps_diag_expected(Parser* ps, const char* thing);
void ps_diag_at(Parser* ps, Pos pos, const char* format, ...);
void ps_skip_past_newline(Parser* ps);

bool parse_literal(Parser* ps);
bool parse_primary(Parser* ps);
bool parse_postfix(Parser* ps);
bool parse_unary(Parser* ps);
bool parse_binary(Parser* ps, u8 min_prec);

#define diag_token(t, ...) ps_diag_at(ps, (t)->pos, __VA_ARGS__)

#define IN_BOUNDS (ps->cur < ps->tokens_len)

#endif // _PARSER_INTERNAL_H

/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _PARSER_H
#define _PARSER_H

#include "ast.h"
#include "common.h"
#include "lexertypes.h"

#define MAX_ERROR_COUNT 20

typedef struct {
    Token* tokens;
    usize tokens_len;
    a_string file_name;
    usize cur;
    u32 error_count;
    bool error_reported;
    bool eof;
    // used as return values
    CB_Stmt stmt;
    CB_Expr expr;
} Parser;

// requires toks to be a valid pointer, ownership of file_name will be taken
Parser ps_new(Token* toks, usize tokens_len, a_string file_name);
void ps_free(Parser* ps);

bool ps_next_expr(Parser* ps);
bool ps_next_stmt(Parser* ps);

#endif // _PARSER_H

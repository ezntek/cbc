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

#include "../ast.h"
#include "../common.h"
#include "../lexer_types.h"

#define MAX_ERROR_COUNT 20

typedef struct {
    Token* tokens;
    usize tokens_len;
    a_string file_name;
    usize cur;
    // used as return values
    CB_Stmt stmt;
    CB_Expr expr;
    u32 error_count;
    // eof marker
    bool eof;
} Parser;

// requires toks to be a valid pointer, ownership of file_name will be taken
Parser ps_new(Token* toks, usize tokens_len, a_string file_name);
void ps_free(Parser* ps);

bool ps_expr(Parser* ps);
bool ps_stmt(Parser* ps);
bool ps_program(Parser* ps, CB_Program* out);

#endif // _PARSER_H

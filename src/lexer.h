/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef CBC_LEXER_H
#define CBC_LEXER_H

#include "a_string_slice.h"
#include "common.h"
#include "error.h"
#include "lexer_types.h"

typedef struct {
    const char* src;
    usize src_len;
    usize cur;
    usize bol;
    u32 row;

    // current token being worked on
    CBCToken token;

    // current error
    CBCError error;
} CBCLexer;

CBCLexer cbc_lexer_new(a_string_slice src);

void cbc_lexer_reset(CBCLexer* l);

// returns NULL on error and sets l->error to a valid value. returns a valid
// pointer to a valid token at l->token when successful.
CBCToken* cbc_lexer_next_token(CBCLexer* l);

// returns length of out buf
usize cbc_lexer_tokenize(CBCLexer* l, CBCToken** out, a_string_slice file_name);

#endif // CBC_LEXER_H

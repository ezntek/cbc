/*
 * cimi: a scuffed scripting language
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2025.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _LEXER_H
#define _LEXER_H

#include "a_string.h"
#include "a_vector.h"
#include "common.h"

#include "lexertypes.h"

AV_DECL(Token, Tokens)

Token token_new_ident(const char* str);
a_string token_kind_to_string(TokenKind k);
void token_print_long(Token* t);
void token_print(Token* t);
Token token_dupe(Token* t);

// free heap allocated data.
// of course, you can still read the kind if you have to.
void token_free(Token* t);

typedef struct {
    const char* src;
    usize src_len;

    // current token (public)
    Token token;

    // error (public)
    LexerError error;

    // internal lexer state
    u32 cur;
    u32 row;
    u32 bol;
} Lexer;

Lexer lx_new(const char* src, usize src_len);
Token* lx_next_token(Lexer* l);
void lx_free(Lexer* l);
void lx_reset(Lexer* l);
char* lx_strerror(LexerErrorKind e);
a_string lx_as_strerror(LexerErrorKind e);
void lx_perror(LexerErrorKind e, const char* pre);
Tokens lx_tokenize(Lexer* l);

#endif // _LEXER_H

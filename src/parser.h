/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef CBC_PARSER_H
#define CBC_PARSER_H

#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "lexer_types.h"

typedef enum {
    CBC_PARSER_MODE_BEANCODE = 0,
    CBC_PARSER_MODE_BEANBEAN,
    CBC_PARSER_MODE_COOKEDBEAN,
} CBCParser_Mode;

AV_DECL(CBCError, CBCParser_Errors);

typedef struct {
    CBCAst__ExprStorage exprs;
    CBCAst__StringStorage strings;
    CBCAst__LiteralStorage literals;
    CBCAst__ArrayLiteralStorage array_literals;
    CBCParser_Errors errors;
    // currently worked on AST
    CBCToken cur, next;
    CBCParser_Mode mode;

    // NOTE: this pointer must stay valid for whenever this parser is valid!
    CBCLexer* lexer;

    bool fatal;
} CBCParser;

CBCParser cbc_parser_new(CBCLexer* l);
void cbc_parser_free(CBCParser* p);
CBCAst cbc_parser_parse(CBCParser* p);

#endif // CBC_PARSER_H

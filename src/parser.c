/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "error.h"
#define _POSIX_C_SOURCE 200809L

#include "a_vector.h"
#include "ast.h"
#include "lexer.h"
#include "lexer_types.h"
#include "parser.h"

static bool consume(CBCParser* p) {
    p->cur = p->next;
    if (p->cur.kind != CBC_TOKEN_EOF) {
        p->next = *cbc_lexer_next_token(p->lexer);
    } else {
        p->fatal = true;
        CBCError e = cbc_error_new(CBC_ERROR_EOF, p->cur.pos);
        av_append(&p->errors, e);
        return false;
    }

    return true;
}

CBCParser cbc_parser_new(CBCLexer* l) {
    return (CBCParser){.lexer = l};
}

void cbc_parser_free(CBCParser* p) {
    for (usize i = 0; i < p->strings.len; i++) {
        // This slice is owned.
        free((void*)p->strings.data[i].data);
    }
    for (usize i = 0; i < p->array_literals.len; i++) {
        free((void*)p->array_literals.data[i].data);
    }
    for (usize i = 0; i < p->errors.len; i++) {
    }
    av_free(&p->exprs);
    av_free(&p->strings);
    av_free(&p->literals);
    av_free(&p->array_literals);
}

CBCAst cbc_parser_parse(CBCParser* p) {
    CBCAst ast = {0};

    return ast;
}

/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define _POSIX_C_SOURCE 200809L

#include "ast.h"
#include "a_string_slice.h"
#include "a_vector.h"

void cbc_ast_free(CBCAst* ast) {
    for (usize i = 0; i < ast->strings.len; i++) {
        // This slice is owned.
        free((void*)ast->strings.data[i].data);
    }
    for (usize i = 0; i < ast->array_literals.len; i++) {
        free((void*)ast->array_literals.data[i].data);
    }
    av_free(&ast->strings);
    av_free(&ast->literals);
    av_free(&ast->array_literals);
    av_free(&ast->exprs);
}

CBCAst_Program cbc_ast_get_program(CBCAst* ast) {
    return ast->prog;
}

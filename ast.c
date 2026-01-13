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

#include "ast.h"
#include "a_string.h"
#include "common.h"
#include <stdlib.h> // macro
#include <string.h> // macro

CB_ArrayType cb_array_type_new(CB_Type kind, const CB_ArrayTypeDim* dims,
                               u16 dims_count) {
    dupe_slice(CB_ArrayTypeDim, new_dims, dims, dims_count);
    return (CB_ArrayType){kind, new_dims, dims_count};
}

void cb_array_type_free(CB_ArrayType* at) {
    free(at->dims);
    at->dims = NULL;
}

CB_Value cb_value_new_null(void) {
    return (CB_Value){
        .kind = CB_PRIM_NULL,
    };
}

CB_Value cb_value_new_integer(i64 integer) {
    return (CB_Value){
        .kind = CB_PRIM_INTEGER,
        .integer = integer,
    };
}

CB_Value cb_value_new_real(f64 real) {
    return (CB_Value){.kind = CB_PRIM_REAL, .real = real};
}

CB_Value cb_value_new_boolean(bool boolean) {
    return (CB_Value){.kind = CB_PRIM_BOOLEAN, .boolean = boolean};
}

CB_Value cb_value_new_char(char chr) {
    return (CB_Value){.kind = CB_PRIM_CHAR, .chr = chr};
}

CB_Value cb_value_new_string(a_string s) {
    a_string ns = as_dupe(&s);
    return (CB_Value){
        .kind = CB_PRIM_STRING,
        .string.data = ns.data,
        .string.len = ns.len,
    };
}

void cb_value_free(CB_Value* v) {
    switch (v->kind) {
        case CB_PRIM_STRING: {
            free((void*)v->string.data);
        } break;
        default: break;
    }
}

CB_Expr cb_expr_new_literal(Pos pos, CB_Value v) {
    return (CB_Expr){
        .kind = CB_EXPR_LIT,
        .pos = pos,
        .lit = v,
    };
}

CB_Expr cb_expr_new_ident(Pos pos, a_string s) {
    return (CB_Expr){
        .kind = CB_EXPR_IDENT,
        .pos = pos,
        .ident = as_dupe(&s),
    };
}

CB_Expr cb_expr_new_unary(Pos pos, CB_ExprKind k, CB_Expr operand) {
    dupe(CB_Expr, n, operand);
    return (CB_Expr){k, .pos = pos, .unary = n};
}

CB_Expr cb_expr_new_binary(Pos pos, CB_ExprKind k, CB_Expr lhs, CB_Expr rhs) {
    dupe(CB_Expr, nlhs, lhs);
    dupe(CB_Expr, nrhs, rhs);
    return (CB_Expr){k, .pos = pos, .lhs = nlhs, .rhs = nrhs};
}

void cb_expr_free(CB_Expr* e) {
    if (e->kind == CB_EXPR_LIT) {
        cb_value_free(&e->lit);
    } else if (e->kind == CB_EXPR_IDENT) {
        as_free(&e->ident);
    } else if (cb_expr_kind_is_unary(e->kind)) {
        cb_expr_free(e->unary);
        free(e->unary);
    } else if (cb_expr_kind_is_binary(e->kind)) {
        cb_expr_free(e->lhs);
        cb_expr_free(e->rhs);
        free(e->lhs);
        free(e->rhs);
    }
}

// takes ownership
CB_OutputStmt cb_output_stmt_new(CB_Expr* exprs, usize exprs_count) {
    return (CB_OutputStmt){
        exprs,
        exprs_count,
    };
}

void cb_output_stmt_free(CB_OutputStmt* s) {
    for (usize i = 0; i < s->exprs_count; i++)
        cb_expr_free(&s->exprs[i]);
    free(s->exprs);
}

CB_InputStmt cb_input_stmt_new(CB_Expr target) {
    dupe(CB_Expr, duped_target, target);
    return (CB_InputStmt){duped_target};
}

void cb_input_stmt_free(CB_InputStmt* s) {
    cb_expr_free(s->target);
    free(s->target);
}

CB_Stmt cb_stmt_new_expr(Pos pos, CB_Expr expr) {
    return (CB_Stmt){CB_STMT_EXPR, pos, .expr = expr};
}

CB_Stmt cb_stmt_new_output(Pos pos, CB_OutputStmt output) {
    return (CB_Stmt){CB_STMT_OUTPUT, pos, .output = output};
}

CB_Stmt cb_stmt_new_input(Pos pos, CB_InputStmt input) {
    return (CB_Stmt){CB_STMT_OUTPUT, pos, .input = input};
}

void cb_stmt_free(CB_Stmt* s) {
    switch (s->kind) {
        case CB_STMT_EXPR: {
            cb_expr_free(&s->expr);
        } break;
        case CB_STMT_OUTPUT: {
            cb_output_stmt_free(&s->output);
        } break;
        case CB_STMT_INPUT: {
            cb_input_stmt_free(&s->input);
        } break;
    }
}

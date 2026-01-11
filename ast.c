/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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

CB_Value cb_value_new_null() {
    return (CB_Value){
        .kind = CB_PRIM_NULL,
    };
}

CB_Value cb_value_new_integer(i64 integer) {
    return (CB_Value){
        .kind = CB_PRIM_INTEGER,
        integer,
    };
}

CB_Value cb_value_new_real(f64 real) {
    return (CB_Value){.kind = CB_PRIM_REAL, real};
}

CB_Value cb_value_new_boolean(bool boolean) {
    return (CB_Value){.kind = CB_PRIM_BOOLEAN, boolean};
}

CB_Value cb_value_new_char(char chr) {
    return (CB_Value){.kind = CB_PRIM_CHAR, chr};
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

CB_Expr cb_expr_new_literal(CB_Value v) {
    return (CB_Expr){
        .kind = CB_EXPR_LIT,
        .lit = v,
    };
}

CB_Expr cb_expr_new_ident(a_string s) {
    return (CB_Expr){
        .kind = CB_EXPR_IDENT,
        .ident = as_dupe(&s),
    };
}

CB_Expr cb_expr_new_unary(CB_ExprKind k, CB_Expr operand) {
    dupe(CB_Expr, n, operand);
    return (CB_Expr){k, .unary = n};
}

CB_Expr cb_expr_new_binary(CB_ExprKind k, CB_Expr lhs, CB_Expr rhs) {
    dupe(CB_Expr, nlhs, lhs);
    dupe(CB_Expr, nrhs, rhs);
    return (CB_Expr){k, .lhs = nlhs, .rhs = nrhs};
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

CB_OutputStmt cb_output_stmt_new(CB_Expr* exprs, usize exprs_count) {
    dupe_slice(CB_Expr, duped_exprs, exprs, exprs_count);
    return (CB_OutputStmt){
        duped_exprs,
        exprs_count,
    };
}

void cb_output_stmt_free(CB_OutputStmt* s) {
    for (usize i = 0; i < s->exprs_count; i++) {
        cb_expr_free(&s->exprs[i]);
        free(&s->exprs[i]);
    }
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

CB_Stmt cb_stmt_new_expr(CB_Expr expr) {
    return (CB_Stmt){CB_STMT_EXPR, .expr = expr};
}

CB_Stmt cb_stmt_new_output(CB_OutputStmt output) {
    return (CB_Stmt){CB_STMT_OUTPUT, .output = output};
}

CB_Stmt cb_stmt_new_input(CB_InputStmt input) {
    return (CB_Stmt){CB_STMT_OUTPUT, .input = input};
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

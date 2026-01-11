/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _AST_H
#define _AST_H

#include "a_string.h"
#include "common.h"

typedef enum {
    CB_PRIM_NULL = 0,
    CB_PRIM_INTEGER,
    CB_PRIM_REAL,
    CB_PRIM_BOOLEAN,
    CB_PRIM_CHAR,
    CB_PRIM_STRING,
    // not for use.
    // (CB_Type)type < CB_PRIM_CUSTOM => primitive type
    // (CB_Type)type > CB_PRIM_CUSTOM => custom type
    CB_PRIM_CUSTOM,
} CB_PrimitiveType;

typedef u32 CB_Type;

typedef union {
    struct {
        u32 begin;
        u32 end;
    };
    // for comparisons
    u64 _v;
} CB_ArrayTypeDim;

typedef struct {
    CB_Type kind;
    CB_ArrayTypeDim* dims;
    u16 dims_count;
} CB_ArrayType;

CB_ArrayType cb_array_type_new(CB_Type kind, const CB_ArrayTypeDim* dims,
                               u16 dims_count);
void cb_array_type_free(CB_ArrayType* at);

typedef struct {
    CB_Type kind;
    union {
        i64 integer;
        f64 real;
        bool boolean;
        char chr; // char
        struct {
            char* data;
            usize len;
        } string;
    };
} CB_Value;

CB_Value cb_value_new_null();
CB_Value cb_value_new_integer(i64 integer);
CB_Value cb_value_new_real(f64 real);
CB_Value cb_value_new_boolean(bool boolean);
CB_Value cb_value_new_char(char chr);
CB_Value cb_value_new_string(a_string s);
void cb_value_free(CB_Value* v);

typedef enum {
    CB_EXPR_LIT = 0,
    CB_EXPR_IDENT,
    // Unaries
    CB_EXPR_NEG,
    CB_EXPR_NOT,
    CB_EXPR_GROUPING,
    CB_EXPR_TYPECAST,
    CB_EXPR_FNCALL,
    // Binaries
    CB_EXPR_ADD,
    CB_EXPR_SUB,
    CB_EXPR_MUL,
    CB_EXPR_DIV,
    CB_EXPR_POW,
    // TODO: add the rest
} CB_ExprKind;

#define cb_expr_kind_is_unary(k)  ((k) <= CB_EXPR_NEG && (k) <= CB_EXPR_FNCALL)
#define cb_expr_kind_is_binary(k) ((k) <= CB_EXPR_ADD && (k) <= CB_EXPR_POW)

typedef struct CB_Expr CB_Expr;
typedef struct CB_Expr {
    CB_ExprKind kind;
    union {
        CB_Value lit;
        a_string ident;
        CB_Expr* unary;
        struct {
            CB_Expr* lhs;
            CB_Expr* rhs;
        };
    };
} CB_Expr;

CB_Expr cb_expr_new_literal(CB_Value v);
CB_Expr cb_expr_new_ident(a_string s);
CB_Expr cb_expr_new_unary(CB_ExprKind k, CB_Expr operand);
CB_Expr cb_expr_new_binary(CB_ExprKind k, CB_Expr lhs, CB_Expr rhs);
void cb_expr_free(CB_Expr* e);

typedef enum {
    CB_STMT_EXPR = 0,
    CB_STMT_OUTPUT,
    CB_STMT_INPUT,
    // TODO: add the rest
} CB_StmtKind;

typedef struct {
    CB_Expr* exprs;
    usize exprs_count;
} CB_OutputStmt;

CB_OutputStmt cb_output_stmt_new(CB_Expr* exprs, usize exprs_count);
void cb_output_stmt_free(CB_OutputStmt* s);

typedef struct {
    // we will figure out if this is an Lvalue later
    CB_Expr* target;
} CB_InputStmt;

CB_InputStmt cb_input_stmt_new(CB_Expr target);
void cb_input_stmt_free(CB_InputStmt* s);

typedef struct {
    CB_StmtKind kind;
    union {
        CB_Expr expr;
        CB_OutputStmt output;
        CB_InputStmt input;
    };
} CB_Stmt;

CB_Stmt cb_stmt_new_expr(CB_Expr expr);
CB_Stmt cb_stmt_new_output(CB_OutputStmt output);
CB_Stmt cb_stmt_new_input(CB_InputStmt input);
void cb_stmt_free(CB_Stmt* s);

#endif // _AST_H

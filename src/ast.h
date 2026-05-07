/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef CBC_AST_H
#define CBC_AST_H

#include "a_string_slice.h"
#include "common.h"

typedef enum {
    CBC_PRIM_INTEGER = 0x00,
    CBC_PRIM_REAL = 0x01,
    CBC_PRIM_FLOAT = 0x02,
    CBC_PRIM_BOOLEAN = 0x03,
    CBC_PRIM_CHAR = 0x04,
    CBC_PRIM_STRING = 0x05,
    CBC_PRIM_I64 = 0x06,
    CBC_PRIM_I32 = 0x07,
    CBC_PRIM_I16 = 0x08,
    CBC_PRIM_I8 = 0x09,
    CBC_PRIM_U64 = 0x0a,
    CBC_PRIM_U32 = 0x0b,
    CBC_PRIM_U16 = 0x0c,
    CBC_PRIM_U8 = 0x0d,
    // >=0x10: custom types
} CBCAst_PrimitiveType;

typedef struct {
    const a_string_slice* names;
    const u32* types;
    u32 field_count;
} CBCAst_StructType;

void cbc_ast_struct_type_free(CBCAst_StructType* t);

typedef struct {
    const a_string_slice* variants;
    // TODO: figure out how to deduce final size of enum
    const struct CBCAst_Literal* items;
    CBCAst_PrimitiveType variant_type;
    u32 variant_count;
} CBCAst_EnumType;

typedef struct {
    const a_string_slice* names;
    const u32* types;
    u32 field_count;
} CBCAst_UnionType;

typedef enum {
    CBC_LITERAL_INTEGER = 0,
    CBC_LITERAL_REAL,
    CBC_LITERAL_BOOLEAN,
    CBC_LITERAL_STRING,
    CBC_LITERAL_CHAR,
    CBC_LITERAL_ARRAY,
} CBCAst_LiteralKind;

// Technically allows for n-dimensional arrays (good)
typedef struct {
    struct CBCAst_Literal* data;
    usize len;
} CBCAst_ArrayLiteral;

typedef struct CBCAst_Literal {
    CBCAst_LiteralKind kind;
    union {
        i64 i;
        double real;
        u8 c;          // BOOLEANs, CHARs,
        u32 string_id; // index into array of owned slices
        u32 array_id;  // index into array of CBCAst_ArrayLiterals
    } v;
} CBCAst_Literal;

typedef struct {

} CBCAst_Typecast;

typedef enum {
    CBC_EXPR_BOGUS = 0x00,
    CBC_EXPR_LITERAL = 0x01,
    CBC_EXPR_IDENT = 0x02,
    CBC_EXPR_FNCALL = 0x03,
    CBC_EXPR_TYPECAST = 0x03,
    // unaries
    // 0001xxxx
    CBC_EXPR_UNOT = 0x10,
    CBC_EXPR_UNEG = 0x11,
    CBC_EXPR_UBITNOT = 0x12,
    CBC_EXPR_UREF = 0x13,
    CBC_EXPR_UDEREF = 0x14,
    // binaries
    // 001xxxxx
    CBC_EXPR_BADD = 0x21,
    CBC_EXPR_BSUB = 0x22,
    CBC_EXPR_BMUL = 0x23,
    CBC_EXPR_BDIV = 0x24,
    CBC_EXPR_BPOW = 0x25,
    CBC_EXPR_BBITAND = 0x26,
    CBC_EXPR_BBITOR = 0x27,
    CBC_EXPR_BBITXOR = 0x28,
    CBC_EXPR_BDOT = 0x29,
    CBC_EXPR_BLT = 0x2a,
    CBC_EXPR_BGT = 0x2b,
    CBC_EXPR_BLEQ = 0x2c,
    CBC_EXPR_BGEQ = 0x2d,
    CBC_EXPR_BEQ = 0x2e,
    CBC_EXPR_BNEQ = 0x2f,
    CBC_EXPR_BSHL = 0x30,
    CBC_EXPR_BSHR = 0x31,
    CBC_EXPR_BOR = 0x32,
    CBC_EXPR_BAND = 0x33,
} CBCAst_ExprKind;

typedef struct {
    CBCAst_ExprKind kind;
    union {
        // index into LiteralStorage
        u32 literal_id;
        // index into StringStorage
        u32 ident_id;
        // index into ExprStorage
        // only use lhs_id for unaries
        u32 lhs_id, rhs_id; // for binaryexprs
        CBCAst_Typecast tc;
    } v;
} CBCAst_Expr;

typedef enum {
    CBC_STMT_BOGUS = 0,
    CBC_STMT_EXPR = 1,
} CBCAst_StmtKind;

typedef struct {
    CBCAst_StmtKind kind;
    union {
        u32 expr_id;
    } v;
} CBCAst_Stmt;

typedef struct {
    const CBCAst_Stmt* stmts;
    usize len;
} CBCAst_Program;

// NOTE: Opaque struct.
// Keep this alive as long as you use the AST. this contains metadata and string
// storage for exprs and such.
typedef struct CBCAst CBCAst;

CBCAst_Program cbc_ast_get_program(CBCAst* ast);
void cbc_ast_free(CBCAst* ast);

#endif

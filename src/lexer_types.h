/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef CBC_LEXER_TYPES_H
#define CBC_LEXER_TYPES_H

#include "a_string.h"
#include "a_string_slice.h"
#include "a_vector.h"
#include "common.h"

typedef struct {
    u32 row;
    u16 col;
    u16 span;
} CBCPos;

// on every call, the result of the previous call is destroyed as we use one
// static buffer
a_string_slice cbc_pos_to_string_slice(const CBCPos* p);

a_string cbc_pos_to_string(const CBCPos* p);

typedef enum {
    CBC_TOKEN_BOGUS = 0,
    CBC_TOKEN_EOF,
    CBC_TOKEN_DECLARE,
    CBC_TOKEN_CONSTANT,
    CBC_TOKEN_OUTPUT,
    CBC_TOKEN_INPUT,
    CBC_TOKEN_AND,
    CBC_TOKEN_OR,
    CBC_TOKEN_NOT,
    CBC_TOKEN_IF,
    CBC_TOKEN_THEN,
    CBC_TOKEN_ELSE,
    CBC_TOKEN_ENDIF,
    CBC_TOKEN_CASE,
    CBC_TOKEN_OF,
    CBC_TOKEN_OTHERWISE,
    CBC_TOKEN_ENDCASE,
    CBC_TOKEN_WHILE,
    CBC_TOKEN_DO,
    CBC_TOKEN_ENDWHILE,
    CBC_TOKEN_REPEAT,
    CBC_TOKEN_UNTIL,
    CBC_TOKEN_FOR,
    CBC_TOKEN_TO,
    CBC_TOKEN_STEP,
    CBC_TOKEN_NEXT,
    CBC_TOKEN_PROCEDURE,
    CBC_TOKEN_ENDPROCEDURE,
    CBC_TOKEN_CALL,
    CBC_TOKEN_FUNCTION,
    CBC_TOKEN_RETURN,
    CBC_TOKEN_RETURNS,
    CBC_TOKEN_ENDFUNCTION,
    CBC_TOKEN_OPENFILE,
    CBC_TOKEN_READFILE,
    CBC_TOKEN_WRITEFILE,
    CBC_TOKEN_CLOSEFILE,
    CBC_TOKEN_READ,
    CBC_TOKEN_WRITE,
    CBC_TOKEN_PRINT,
    CBC_TOKEN_ASSIGN,
    CBC_TOKEN_EQ,
    CBC_TOKEN_LT,
    CBC_TOKEN_GT,
    CBC_TOKEN_LEQ,
    CBC_TOKEN_GEQ,
    CBC_TOKEN_NEQ,
    CBC_TOKEN_MUL,
    CBC_TOKEN_DIV,
    CBC_TOKEN_ADD,
    CBC_TOKEN_SUB,
    CBC_TOKEN_LPAREN,
    CBC_TOKEN_RPAREN,
    CBC_TOKEN_LBRACKET,
    CBC_TOKEN_RBRACKET,
    CBC_TOKEN_LCURLY,
    CBC_TOKEN_RCURLY,
    CBC_TOKEN_COLON,
    CBC_TOKEN_COMMA,
    CBC_TOKEN_DOT,
    CBC_TOKEN_NEWLINE,
    CBC_TOKEN_LIT_STRING,
    CBC_TOKEN_LIT_CHAR,
    CBC_TOKEN_LIT_NUMBER,
    CBC_TOKEN_TRUE,
    CBC_TOKEN_FALSE,
    CBC_TOKEN_NULL,
    CBC_TOKEN_IDENT,
    CBC_TOKEN_INTEGER,
    CBC_TOKEN_BOOLEAN,
    CBC_TOKEN_REAL,
    CBC_TOKEN_CHAR,
    CBC_TOKEN_STRING,
    CBC_TOKEN_ARRAY,

    // StandardBean extensions
    CBC_TOKEN_END,
    CBC_TOKEN_STRUCT,
    CBC_TOKEN_ENUM,
    CBC_TOKEN_UNION,
    CBC_TOKEN_ENDSTRUCT,
    CBC_TOKEN_ENDENUM,
    CBC_TOKEN_ENDUNION,
    CBC_TOKEN_BITAND,
    CBC_TOKEN_BITOR,
    CBC_TOKEN_BITNOT,
    CBC_TOKEN_BITXOR,
    CBC_TOKEN_CARET,
    CBC_TOKEN_I8,
    CBC_TOKEN_I16,
    CBC_TOKEN_I32,
    CBC_TOKEN_I64,
    CBC_TOKEN_U8,
    CBC_TOKEN_U16,
    CBC_TOKEN_U32,
    CBC_TOKEN_U64,
    CBC_TOKEN_FLOAT,
    CBC_TOKEN_IMPORT,
    CBC_TOKEN_AS,
} CBCTokenKind;

// on every call, the result of the previous call is destroyed as we use one
// static buffer
a_string_slice cbc_token_kind_to_string_slice(CBCTokenKind k);

a_string cbc_token_kind_to_string(CBCTokenKind k);

typedef struct {
    CBCTokenKind kind;
    CBCPos pos;
    // index into src string
    u32 src_index;
} CBCToken;

AV_DECL(CBCToken, CBCTokenArray)

// on every call, the result of the previous call is destroyed as we use one
// static buffer
a_string_slice cbc_token_to_string_slice(const CBCToken* t);

a_string cbc_token_to_string(const CBCToken* t);

a_string_slice cbc_token_to_string_slice_full(const CBCToken* t,
                                              const a_string_slice src);

a_string cbc_token_to_string_full(const CBCToken* t, const a_string_slice src);

#endif // CBC_LEXER_TYPES_H

/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _LEXER_TYPES_H
#define _LEXER_TYPES_H

#include "a_string.h"
#include "common.h"

typedef struct {
    u32 row;
    u16 file_id;
    u8 col;
    u8 span;
} Pos;

typedef enum {
    LX_ERROR_NULL = 0,
    LX_ERROR_UNTERMINATED_LITERAL,
    LX_ERROR_EOF,
    LX_ERROR_BAD_ESCAPE,
    LX_ERROR_MISMATCHED_DELIMITER,
    LX_ERROR_INVALID_IDENTIFIER,
    LX_ERROR_CHAR_LITERAL_TOO_LONG,
} LexerErrorKind;

typedef struct {
    LexerErrorKind kind;
    Pos pos;
} LexerError;

typedef enum {
    TOK_IDENT = 0,
    TOK_EOF,
    TOK_INVALID, // internal use only!!
    TOK_NEWLINE,

    // Literals
    TOK_LITERAL_STRING,
    TOK_LITERAL_CHAR,
    TOK_LITERAL_NUMBER,
    TOK_LITERAL_BOOLEAN,

    // Keywords
    TOK_DECLARE,
    TOK_CONSTANT,
    TOK_OUTPUT,
    TOK_PRINT,
    TOK_INPUT,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_ENDIF,
    TOK_CASE,
    TOK_OF,
    TOK_OTHERWISE,
    TOK_ENDCASE,
    TOK_WHILE,
    TOK_DO,
    TOK_ENDWHILE,
    TOK_FOR,
    TOK_TO,
    TOK_STEP,
    TOK_NEXT,
    TOK_FUNCTION,
    TOK_RETURNS,
    TOK_ENDFUNCTION,
    TOK_PROCEDURE,
    TOK_ENDPROCEDURE,
    TOK_RETURN,
    TOK_INCLUDE,
    TOK_EXPORT,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_REPEAT,
    TOK_UNTIL,
    TOK_STRUCT,
    TOK_ENDSTRUCT,

    // Types
    TOK_INTEGER,
    TOK_REAL,
    TOK_BOOLEAN,
    TOK_STRING,
    TOK_CHAR,
    TOK_NULL,

    // Separators
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LCURLY,
    TOK_RCURLY,
    TOK_COMMA,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_DOT,

    // Operators
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_CARET,
    TOK_LT,
    TOK_GT,
    TOK_LEQ,
    TOK_GEQ,
    TOK_EQ,
    TOK_NEQ,
    TOK_ASSIGN,
    TOK_SHR,
    TOK_SHL,
    TOK_BITOR,
    TOK_BITAND,
    TOK_BITNOT,
    TOK_BITXOR,
} TokenKind;

typedef struct {
    TokenKind kind;
    Pos pos;
    union {
        a_string string; // idents, other literals
        bool boolean;    // bool literals
    } data;
} Token;

#endif // _LEXER_TYPES_H

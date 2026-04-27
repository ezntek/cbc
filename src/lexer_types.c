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

#include <assert.h>
#include <stdio.h>
// macro
#include <string.h>

#include "a_string.h"
#include "a_string_slice.h"
#include "lexer_types.h"

static const char* TOKEN_KIND_TABLE[] = {
    [CBC_TOKEN_BOGUS] = "!!! bogus amogus token !!!",
    [CBC_TOKEN_EOF] = "eof",
    [CBC_TOKEN_DECLARE] = "declare",
    [CBC_TOKEN_CONSTANT] = "constant",
    [CBC_TOKEN_OUTPUT] = "output",
    [CBC_TOKEN_INPUT] = "input",
    [CBC_TOKEN_AND] = "and",
    [CBC_TOKEN_OR] = "or",
    [CBC_TOKEN_NOT] = "not",
    [CBC_TOKEN_IF] = "if",
    [CBC_TOKEN_THEN] = "then",
    [CBC_TOKEN_ELSE] = "else",
    [CBC_TOKEN_ENDIF] = "endif",
    [CBC_TOKEN_CASE] = "case",
    [CBC_TOKEN_OF] = "of",
    [CBC_TOKEN_OTHERWISE] = "otherwise",
    [CBC_TOKEN_ENDCASE] = "endcase",
    [CBC_TOKEN_WHILE] = "while",
    [CBC_TOKEN_DO] = "do",
    [CBC_TOKEN_ENDWHILE] = "endwhile",
    [CBC_TOKEN_REPEAT] = "repeat",
    [CBC_TOKEN_UNTIL] = "until",
    [CBC_TOKEN_FOR] = "for",
    [CBC_TOKEN_TO] = "to",
    [CBC_TOKEN_STEP] = "step",
    [CBC_TOKEN_NEXT] = "next",
    [CBC_TOKEN_PROCEDURE] = "procedure",
    [CBC_TOKEN_ENDPROCEDURE] = "endprocedure",
    [CBC_TOKEN_CALL] = "call",
    [CBC_TOKEN_FUNCTION] = "function",
    [CBC_TOKEN_RETURN] = "return",
    [CBC_TOKEN_RETURNS] = "returns",
    [CBC_TOKEN_ENDFUNCTION] = "endfunction",
    [CBC_TOKEN_OPENFILE] = "openfile",
    [CBC_TOKEN_READFILE] = "readfile",
    [CBC_TOKEN_WRITEFILE] = "writefile",
    [CBC_TOKEN_CLOSEFILE] = "closefile",
    [CBC_TOKEN_READ] = "read",
    [CBC_TOKEN_WRITE] = "write",
    [CBC_TOKEN_PRINT] = "print",
    [CBC_TOKEN_ASSIGN] = "assign",
    [CBC_TOKEN_EQ] = "eq",
    [CBC_TOKEN_LT] = "lt",
    [CBC_TOKEN_GT] = "gt",
    [CBC_TOKEN_LEQ] = "leq",
    [CBC_TOKEN_GEQ] = "geq",
    [CBC_TOKEN_NEQ] = "neq",
    [CBC_TOKEN_MUL] = "mul",
    [CBC_TOKEN_DIV] = "div",
    [CBC_TOKEN_ADD] = "add",
    [CBC_TOKEN_SUB] = "sub",
    [CBC_TOKEN_LPAREN] = "lparen",
    [CBC_TOKEN_RPAREN] = "rparen",
    [CBC_TOKEN_LBRACKET] = "lbracket",
    [CBC_TOKEN_RBRACKET] = "rbracket",
    [CBC_TOKEN_LCURLY] = "lcurly",
    [CBC_TOKEN_RCURLY] = "rcurly",
    [CBC_TOKEN_COLON] = "colon",
    [CBC_TOKEN_COMMA] = "comma",
    [CBC_TOKEN_DOT] = "dot",
    [CBC_TOKEN_NEWLINE] = "newline",
    [CBC_TOKEN_LIT_STRING] = "lit_string",
    [CBC_TOKEN_LIT_CHAR] = "lit_char",
    [CBC_TOKEN_LIT_NUMBER] = "lit_number",
    [CBC_TOKEN_TRUE] = "true",
    [CBC_TOKEN_FALSE] = "false",
    [CBC_TOKEN_NULL] = "null",
    [CBC_TOKEN_IDENT] = "ident",
    [CBC_TOKEN_INTEGER] = "integer",
    [CBC_TOKEN_BOOLEAN] = "boolean",
    [CBC_TOKEN_REAL] = "real",
    [CBC_TOKEN_CHAR] = "char",
    [CBC_TOKEN_STRING] = "string",
    [CBC_TOKEN_ARRAY] = "array",

    // StandardBean extensions
    [CBC_TOKEN_END] = "end",
    [CBC_TOKEN_STRUCT] = "struct",
    [CBC_TOKEN_ENUM] = "enum",
    [CBC_TOKEN_UNION] = "union",
    [CBC_TOKEN_ENDSTRUCT] = "endstruct",
    [CBC_TOKEN_ENDENUM] = "endenum",
    [CBC_TOKEN_ENDUNION] = "endunion",
    [CBC_TOKEN_BITAND] = "bitand",
    [CBC_TOKEN_BITOR] = "bitor",
    [CBC_TOKEN_BITNOT] = "bitnot",
    [CBC_TOKEN_BITXOR] = "bitxor",
    [CBC_TOKEN_CARET] = "caret",
    [CBC_TOKEN_I8] = "i8",
    [CBC_TOKEN_I16] = "i16",
    [CBC_TOKEN_I32] = "i32",
    [CBC_TOKEN_I64] = "i64",
    [CBC_TOKEN_U8] = "u8",
    [CBC_TOKEN_U16] = "u16",
    [CBC_TOKEN_U32] = "u32",
    [CBC_TOKEN_U64] = "u64",
    [CBC_TOKEN_FLOAT] = "float",
    [CBC_TOKEN_IMPORT] = "import",
    [CBC_TOKEN_AS] = "as",
};

static char cbc_token_full_buf[512] = {0};
static char cbc_pos_buf[256] = {0};
static char cbc_token_buf[256] = {0};

a_string_slice cbc_pos_to_string_slice(const CBCPos* p) {
    usize len = snprintf(cbc_pos_buf, sizeof(cbc_pos_buf), "%u %u %u", p->row,
                         p->col, p->span);
    return (a_string_slice){.data = cbc_pos_buf, .len = len};
}

a_string cbc_pos_to_string(const CBCPos* p) {
    return as_asprintf("[%u %u %u]", p->row, p->col, p->span);
}

a_string_slice cbc_token_kind_to_string_slice(CBCTokenKind k) {
    return ass_from_cstr(TOKEN_KIND_TABLE[k]);
}

a_string cbc_token_kind_to_string(CBCTokenKind k) {
    return astr(TOKEN_KIND_TABLE[k]);
}

a_string_slice cbc_token_to_string_slice(const CBCToken* t) {
    a_string_slice k = cbc_token_kind_to_string_slice(t->kind),
                   p = cbc_pos_to_string_slice(&t->pos);

    usize len = snprintf(cbc_token_buf, sizeof(cbc_token_buf),
                         "token[%.*s]: %.*s", as_fmt(p), as_fmt(k));

    return (a_string_slice){.data = cbc_token_buf, .len = len};
}

a_string cbc_token_to_string(const CBCToken* t) {
    return as_from_string_slice(cbc_token_to_string_slice(t));
}

a_string_slice cbc_token_to_string_slice_full(const CBCToken* t,
                                              const a_string_slice src) {
    a_string_slice k = cbc_token_kind_to_string_slice(t->kind),
                   p = cbc_pos_to_string_slice(&t->pos);
    usize len = 0;

    switch (t->kind) {
        case CBC_TOKEN_LIT_CHAR:
        case CBC_TOKEN_LIT_NUMBER:
        case CBC_TOKEN_LIT_STRING:
        case CBC_TOKEN_IDENT: {
            assert(ass_valid(src));
            assert(t->src_index + t->pos.span <= src.len);
            len = snprintf(cbc_token_full_buf, sizeof(cbc_token_buf),
                           "token[%.*s]: {%.*s}", as_fmt(p), (int)t->pos.span,
                           src.data + (usize)t->src_index);
        } break;
        default: {
            len = snprintf(cbc_token_full_buf, sizeof(cbc_token_buf),
                           "token[%.*s]: <%.*s>", as_fmt(p), as_fmt(k));
        } break;
    }

    return (a_string_slice){.data = cbc_token_full_buf, .len = len};
}

a_string cbc_token_to_string_full(const CBCToken* t, const a_string_slice src) {
    return as_from_string_slice(cbc_token_to_string_slice_full(t, src));
}

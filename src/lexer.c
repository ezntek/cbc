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

#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "a_string_slice.h"
#include "a_vector.h"
#include "error.h"
#include "lexer.h"
#include "lexer_types.h"

AV_DECL(CBCToken, Tokens)

#define CUR       (l->src[l->cur])
#define IN_BOUNDS (l->cur < l->src_len)
#define POS(sp)                                                                \
    (CBCPos) {                                                                 \
        .row = l->row, .col = l->cur - l->bol + 1 - (sp), .span = (sp)         \
    }

#define POS_HERE(sp)                                                           \
    (CBCPos) {                                                                 \
        .row = l->row, .col = l->cur - l->bol + 1, .span = (sp)                \
    }

#define BUMP_NEWLINE                                                           \
    do {                                                                       \
        l->bol = ++l->cur;                                                     \
        l->row++;                                                              \
    } while (0)

static CBCTokenKind token_kind_from_single_op(char ch);
static CBCTokenKind token_kind_from_multi_op(const a_string_slice s);
static CBCTokenKind token_kind_from_keyword(const a_string_slice s);

static void trim_spaces(CBCLexer* l);
static void trim_comments(CBCLexer* l);
static bool is_number(a_string_slice word);
static bool is_ident(a_string_slice word);
static bool is_operator_start(CBCLexer* l, const char* start);
static bool is_separator(char ch);

static bool next_word(CBCLexer* l, a_string_slice* out);
static bool next_multi_symbol(CBCLexer* l);
static bool next_single_symbol(CBCLexer* l);
static bool next_keyword(CBCLexer* l, a_string_slice word);
static bool next_literal(CBCLexer* l, a_string_slice word);
static bool next_ident(CBCLexer* l, a_string_slice word);

static CBCTokenKind token_kind_from_single_op(char ch) {
    switch (ch) {
        case '{': return CBC_TOKEN_LCURLY;
        case '}': return CBC_TOKEN_RCURLY;
        case '[': return CBC_TOKEN_LBRACKET;
        case ']': return CBC_TOKEN_RBRACKET;
        case '(': return CBC_TOKEN_LPAREN;
        case ')': return CBC_TOKEN_RPAREN;
        case ':': return CBC_TOKEN_COLON;
        case ';': return CBC_TOKEN_NEWLINE;
        case ',': return CBC_TOKEN_COMMA;
        case '=': return CBC_TOKEN_EQ;
        case '<': return CBC_TOKEN_LT;
        case '>': return CBC_TOKEN_GT;
        case '*': return CBC_TOKEN_MUL;
        case '/': return CBC_TOKEN_DIV;
        case '+': return CBC_TOKEN_ADD;
        case '-': return CBC_TOKEN_SUB;
        case '^': return CBC_TOKEN_CARET;
        case '.': return CBC_TOKEN_DOT;
        default: return CBC_TOKEN_BOGUS;
    }
}

static CBCTokenKind token_kind_from_multi_op(const a_string_slice s) {
    if (!s.len)
        return CBC_TOKEN_BOGUS;
// NOTE: we already did a bounds check in next_multi_symbol
#define is(x) (!strncmp(s.data, (x), s.len))
    // U+2190 ←
    if (is("\xe2\x86\x90"))
        return CBC_TOKEN_ASSIGN;
    // U+F0AC
    if (is("\xef\x82\xac"))
        return CBC_TOKEN_ASSIGN;
    if (is("<-"))
        return CBC_TOKEN_ASSIGN;
    if (is("<>"))
        return CBC_TOKEN_NEQ;
    if (is("!="))
        return CBC_TOKEN_NEQ;
    if (is(">="))
        return CBC_TOKEN_GEQ;
    if (is("<="))
        return CBC_TOKEN_LEQ;
    if (is("=="))
        return CBC_TOKEN_EQ;

    return CBC_TOKEN_BOGUS;
#undef is
}

static CBCTokenKind token_kind_from_keyword(const a_string_slice s) {
#define is(x) (ass_equal_nocase_cstr(s, (x)))
    if (is("declare") || is("var"))
        return CBC_TOKEN_DECLARE;
    if (is("constant") || is("const"))
        return CBC_TOKEN_CONSTANT;
    if (is("output"))
        return CBC_TOKEN_OUTPUT;
    if (is("input"))
        return CBC_TOKEN_INPUT;
    if (is("and"))
        return CBC_TOKEN_AND;
    if (is("or"))
        return CBC_TOKEN_OR;
    if (is("not"))
        return CBC_TOKEN_NOT;
    if (is("if"))
        return CBC_TOKEN_IF;
    if (is("then"))
        return CBC_TOKEN_THEN;
    if (is("else"))
        return CBC_TOKEN_ELSE;
    if (is("endif"))
        return CBC_TOKEN_ENDIF;
    if (is("case"))
        return CBC_TOKEN_CASE;
    if (is("of"))
        return CBC_TOKEN_OF;
    if (is("otherwise"))
        return CBC_TOKEN_OTHERWISE;
    if (is("endcase"))
        return CBC_TOKEN_ENDCASE;
    if (is("while"))
        return CBC_TOKEN_WHILE;
    if (is("do"))
        return CBC_TOKEN_DO;
    if (is("endwhile"))
        return CBC_TOKEN_ENDWHILE;
    if (is("repeat"))
        return CBC_TOKEN_REPEAT;
    if (is("until"))
        return CBC_TOKEN_UNTIL;
    if (is("for"))
        return CBC_TOKEN_FOR;
    if (is("to"))
        return CBC_TOKEN_TO;
    if (is("step"))
        return CBC_TOKEN_STEP;
    if (is("next"))
        return CBC_TOKEN_NEXT;
    if (is("procedure") || is("proc"))
        return CBC_TOKEN_PROCEDURE;
    if (is("endprocedure"))
        return CBC_TOKEN_ENDPROCEDURE;
    if (is("call"))
        return CBC_TOKEN_CALL;
    if (is("function") || is("fn"))
        return CBC_TOKEN_FUNCTION;
    if (is("returns"))
        return CBC_TOKEN_RETURNS;
    if (is("return"))
        return CBC_TOKEN_RETURN;
    if (is("endfunction"))
        return CBC_TOKEN_ENDFUNCTION;
    if (is("openfile"))
        return CBC_TOKEN_OPENFILE;
    if (is("readfile"))
        return CBC_TOKEN_READFILE;
    if (is("writefile"))
        return CBC_TOKEN_WRITEFILE;
    if (is("closefile"))
        return CBC_TOKEN_CLOSEFILE;
    if (is("read"))
        return CBC_TOKEN_READ;
    if (is("write"))
        return CBC_TOKEN_WRITE;
    if (is("print"))
        return CBC_TOKEN_PRINT;
    if (is("integer") || is("int"))
        return CBC_TOKEN_INTEGER;
    if (is("boolean") || is("bool"))
        return CBC_TOKEN_BOOLEAN;
    if (is("real") || is("double"))
        return CBC_TOKEN_REAL;
    if (is("char"))
        return CBC_TOKEN_CHAR;
    if (is("string"))
        return CBC_TOKEN_STRING;
    if (is("array"))
        return CBC_TOKEN_ARRAY;

    // StandardBean extensions
    if (is("end"))
        return CBC_TOKEN_END;
    if (is("struct"))
        return CBC_TOKEN_STRUCT;
    if (is("enum"))
        return CBC_TOKEN_ENUM;
    if (is("union"))
        return CBC_TOKEN_UNION;
    if (is("endstruct"))
        return CBC_TOKEN_ENDSTRUCT;
    if (is("endenum"))
        return CBC_TOKEN_ENDENUM;
    if (is("endunion"))
        return CBC_TOKEN_ENDUNION;
    if (is("bitand"))
        return CBC_TOKEN_BITAND;
    if (is("bitor"))
        return CBC_TOKEN_BITOR;
    if (is("bitnot"))
        return CBC_TOKEN_BITNOT;
    if (is("bitxor"))
        return CBC_TOKEN_BITXOR;
    if (is("float"))
        return CBC_TOKEN_PRINT;
    if (is("import"))
        return CBC_TOKEN_IMPORT;
    if (is("as"))
        return CBC_TOKEN_AS;
    if (is("i8"))
        return CBC_TOKEN_I8;
    if (is("i16"))
        return CBC_TOKEN_I16;
    if (is("i32"))
        return CBC_TOKEN_I32;
    if (is("i64"))
        return CBC_TOKEN_I64;
    if (is("u8"))
        return CBC_TOKEN_U8;
    if (is("u16"))
        return CBC_TOKEN_U16;
    if (is("u32"))
        return CBC_TOKEN_U32;
    if (is("u64"))
        return CBC_TOKEN_U64;

    return CBC_TOKEN_BOGUS;
#undef is
}

static void trim_spaces(CBCLexer* l) {
    if (!IN_BOUNDS)
        return;

    while (IN_BOUNDS && isspace(CUR) && CUR != '\n')
        l->cur++;

    trim_comments(l);
}

static void trim_comments(CBCLexer* l) {
    if (l->cur + 2 > l->src_len)
        return;

    if (!strncmp(&CUR, "/*", 2)) {
        l->cur += 2;

        while (IN_BOUNDS && strncmp(&CUR, "/*", 2)) {
            if (CUR == '\n')
                BUMP_NEWLINE;
            else
                l->cur++;
        }

        l->cur++;
        trim_spaces(l);
    } else if (!strncmp(&CUR, "//", 2) || !strncmp(&CUR, "#!", 2)) {
        l->cur += 2;

        while (IN_BOUNDS && CUR != '\n')
            l->cur++;

        trim_spaces(l);
    }
}

static bool is_number(a_string_slice word) {
    bool found_decimal = false;
    char cur;

    for (usize i = 0; i < word.len; i++) {
        cur = word.data[i];
        if (isdigit(cur))
            continue;

        if (cur == '.') {
            if (found_decimal)
                return false;
            else
                found_decimal = true;

            continue;
        }

        return false;
    }

    if (found_decimal && word.len == 1)
        return false;

    return true;
}

static bool is_ident(a_string_slice word) {
    if (!isalpha(*word.data) && *word.data == '_')
        return false;

    char cur;
    for (usize i = 0; i < word.len; i++) {
        cur = word.data[i];
        if (!isalnum(cur) && cur != '_' && cur != '.')
            return false;
    }

    return true;
}

static bool is_operator_start(CBCLexer* l, const char* start) {
    // NOTE: we catch % for error
    if (strchr("%+-*/<>=^", *start) != NULL)
        return true;

    if (l->cur + 1 >= l->src_len)
        return false;

    // we just scan the whole thing to avoid a bug where ! does not parse as a
    // word but also not as an operator
    if (!strncmp(&CUR, "!=", 2))
        return true;

    if (l->cur + 2 >= l->src_len)
        return false;

    // U+2190 ← , U+F0AC
    // NOTE: we have to avoid signed character shenanigans.
#define is(s) (!memcmp((u8*)&CUR, (u8*)(s), 3))
    if (is("\xe2\x86\x90"))
        return true;

    if (is("\xef\x82\xac"))
        return true;
#undef is

    return false;
}

static bool is_separator(char ch) {
    return strchr("[]{}();:,.", ch) != NULL;
}

static bool next_word(CBCLexer* l, a_string_slice* out) {
    static const char* DELIMS = "\"'";

    usize begin = l->cur, len = 0;
    char cur, first = l->src[begin];
    bool stop = false, is_delimited = strchr(DELIMS, first);
    bool maybe_number = isdigit(first);

    if (is_delimited) {
        len++;
        l->cur++;
    }

    do {
        if (!IN_BOUNDS)
            break;

        cur = CUR;
        if (is_delimited)
            stop = (cur == first || cur == '\n');
        else
            stop = (is_operator_start(l, &CUR) ||
                    // if it could be a number and it's a dot, treat it as a
                    // decimal
                    (is_separator(cur) && !(cur == '.' && maybe_number)) ||
                    isspace(cur) || strchr(DELIMS, cur));

        if (cur == '\\') {
            len++;
            cur++;
        }

        if (stop)
            break;

        len++;
        l->cur++;
    } while (true);

    if (is_delimited) {
        if (!IN_BOUNDS || isspace(cur)) {
            l->error = cbc_error_new_cstr(
                CBC_ERROR_SYNTAX, POS(len),
                "could not find ending delimiter in literal");
            return false;
        }

        len++;
        l->cur++;
    }

    out->len = len;
    out->data = &l->src[begin];

    return true;
}

static bool next_multi_symbol(CBCLexer* l) {
    if (!is_operator_start(l, &CUR))
        return false;

    a_string_slice chunk = {.data = &CUR};
    for (int s = 3; s >= 2; s--) {
        if (l->cur + (s - 1) < l->src_len)
            chunk.len = s;

        CBCTokenKind k = token_kind_from_multi_op(chunk);
        if (k != CBC_TOKEN_BOGUS) {
            l->cur += s;
            l->token = (CBCToken){
                .kind = k,
                .pos = POS(s),
            };
            return true;
        }
    }

    return false;
}

static bool next_single_symbol(CBCLexer* l) {
    if (!is_operator_start(l, &CUR) && !is_separator(CUR))
        return false;

    CBCTokenKind k = token_kind_from_single_op(CUR);

    if (k != CBC_TOKEN_BOGUS) {
        l->cur++;
        l->token = (CBCToken){
            .kind = k,
            .pos = POS(1),
        };
        return true;
    }

    return false;
}

static bool next_keyword(CBCLexer* l, a_string_slice word) {
    if (!ass_case_consistent(word))
        return false;

    CBCPos p = POS(word.len);
    CBCTokenKind k = token_kind_from_keyword(word);
    if (k != CBC_TOKEN_BOGUS) {
        l->token = (CBCToken){
            .kind = k,
            .pos = p,
        };
        return true;
    }

    if (ass_equal_nocase_cstr(word, "endfor")) {
        l->error = cbc_error_new_cstr(
            CBC_ERROR_SYNTAX, p,
            "ENDFOR is not a valid keyword!\nPlease use NEXT "
            "<your counter> to end a FOR loop instead.");
        return false;
    }

    return false;
}

static bool next_literal(CBCLexer* l, a_string_slice word) {
    if (ass_first(word) == '"' || ass_first(word) == '\'') {
        if (word.len == 1)
            panic("unreachable code");

        CBCTokenKind k =
            ass_first(word) == '"' ? CBC_TOKEN_LIT_STRING : CBC_TOKEN_LIT_CHAR;

        l->token = (CBCToken){
            .src_index = (u32)l->cur - word.len,
            .kind = k,
            .pos = POS(word.len),
        };
        return true;
    }

    if (is_number(word)) {
        l->token = (CBCToken){
            .src_index = (u32)l->cur - word.len,
            .kind = CBC_TOKEN_LIT_NUMBER,
            .pos = POS(word.len),
        };
        return true;
    } else if (isdigit(ass_first(word))) {
        l->error = cbc_error_new_cstr(CBC_ERROR_SYNTAX, POS(word.len),
                                      "invalid number literal");
        return false;
    }

    if (ass_case_consistent(word)) {
        l->token.pos = POS(word.len);
        if (ass_equal_nocase_cstr(word, "true")) {
            l->token.kind = CBC_TOKEN_TRUE;
            return true;
        } else if (ass_equal_nocase_cstr(word, "false")) {
            l->token.kind = CBC_TOKEN_FALSE;
            return true;
        } else if (ass_equal_nocase_cstr(word, "null")) {
            l->token.kind = CBC_TOKEN_NULL;
            return true;
        }
    }

    return false;
}

static bool next_ident(CBCLexer* l, a_string_slice word) {
    CBCPos p = POS(word.len);

    if (is_ident(word)) {
        l->token = (CBCToken){
            .src_index = (u32)l->cur - word.len,
            .kind = CBC_TOKEN_IDENT,
            .pos = p,
        };
        return true;
    } else {
        l->error = cbc_error_new_cstr(CBC_ERROR_SYNTAX, p,
                                      "invalid identifier or symbol");
        return false;
    }
}

// public API

CBCLexer cbc_lexer_new(a_string_slice src) {
    CBCLexer l = {.src = src.data, .src_len = src.len};

    cbc_lexer_reset(&l);

    return l;
}

void cbc_lexer_reset(CBCLexer* l) {
    l->row = 1;
    l->cur = 0;
    l->bol = 0;
    l->token = (CBCToken){0};
    l->error = (CBCError){0};
}

CBCToken* cbc_lexer_next_token(CBCLexer* l) {
    trim_spaces(l);

    if (!IN_BOUNDS) {
        l->token = (CBCToken){
            .kind = CBC_TOKEN_EOF,
            .pos = POS(1),
        };
        return &l->token;
    }

    if (CUR == '\n') {
        l->token = (CBCToken){
            .kind = CBC_TOKEN_NEWLINE,
            .pos = POS(1),
        };
        BUMP_NEWLINE;
        return &l->token;
    }

    if (next_multi_symbol(l))
        return &l->token;
    if (next_single_symbol(l))
        return &l->token;

    a_string_slice word = {0};
    if (!next_word(l, &word))
        return NULL;

    if (next_keyword(l, word))
        return &l->token;
    if (next_literal(l, word))
        return &l->token;
    if (next_ident(l, word))
        return &l->token;

    return NULL;
}

usize cbc_lexer_tokenize(CBCLexer* l, CBCToken** out) {
    Tokens res = {0};
    CBCToken* tok = NULL;

    cbc_lexer_reset(l);

    do {
        tok = cbc_lexer_next_token(l);
        if (!tok) {
            *out = NULL;
            if (res.cap)
                av_free(&res);
            return 0;
        }

        av_append(&res, *tok);
    } while (!tok || tok->kind != CBC_TOKEN_EOF);

    *out = res.data;
    return res.len;
}

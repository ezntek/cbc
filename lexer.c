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

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "3rdparty/uthash.h"
#include "a_string.h"
#include "common.h"
#include "lexer.h"
#include "lexer_types.h"

#define CUR  (l->src[l->cur])
#define PEEK (l->src[l->cur + 1])
#define BUMP_NEWLINE                                                           \
    do {                                                                       \
        l->row++;                                                              \
        l->bol = ++l->cur;                                                     \
    } while (0)
#define IN_BOUNDS (l->cur < l->src_len)
#define POS(sp)                                                                \
    (Pos) {                                                                    \
        .row = l->row, .col = l->cur - l->bol + 1 - (sp), .span = (sp)         \
    }

#define POS_HERE(sp)                                                           \
    (Pos) {                                                                    \
        .row = l->row, .col = l->cur - l->bol + 1, .span = (sp)                \
    }

#define TOKEN(kfull, sp)                                                       \
    (Token) {                                                                  \
        .kind = (kfull), .pos = (POS(sp))                                      \
    }

#define TOK(k, sp)                                                             \
    (Token) {                                                                  \
        .kind = TOK_##k, .pos = (POS(sp))                                      \
    }

#define ERROR(k, sp)                                                           \
    (LexerError) {                                                             \
        .kind = LX_ERROR_##k, .pos = (POS(sp))                                 \
    }

#define TRY(expr)                                                              \
    do {                                                                       \
        if ((expr)) {                                                          \
            goto done;                                                         \
        } else if (l->error.kind != LX_ERROR_NULL) {                           \
            return NULL;                                                       \
        }                                                                      \
    } while (0)

#define TRY_AND_FREE(expr)                                                     \
    do {                                                                       \
        if ((expr)) {                                                          \
            goto next_token_error;                                             \
        } else if (l->error.kind != LX_ERROR_NULL) {                           \
            as_free(&word);                                                    \
            return NULL;                                                       \
        }                                                                      \
    } while (0)

static const char* TOKEN_STRINGS[] = {
    [TOK_IDENT] = "ident",
    [TOK_EOF] = "eof",
    [TOK_INVALID] = "invalid",
    [TOK_NEWLINE] = "newline",
    [TOK_LITERAL_STRING] = "literal_string",
    [TOK_LITERAL_CHAR] = "literal_char",
    [TOK_LITERAL_NUMBER] = "literal_number",
    [TOK_LITERAL_BOOLEAN] = "literal_boolean",
    [TOK_DECLARE] = "DECLARE",
    [TOK_CONSTANT] = "CONSTANT",
    [TOK_OUTPUT] = "OUTPUT",
    [TOK_PRINT] = "PRINT",
    [TOK_INPUT] = "INPUT",
    [TOK_AND] = "AND",
    [TOK_OR] = "OR",
    [TOK_NOT] = "NOT",
    [TOK_IF] = "IF",
    [TOK_THEN] = "THEN",
    [TOK_ELSE] = "ELSE",
    [TOK_ENDIF] = "ENDIF",
    [TOK_CASE] = "CASE",
    [TOK_OF] = "OF",
    [TOK_OTHERWISE] = "OTHERWISE",
    [TOK_ENDCASE] = "ENDCASE",
    [TOK_WHILE] = "WHILE",
    [TOK_DO] = "DO",
    [TOK_ENDWHILE] = "ENDWHILE",
    [TOK_FOR] = "FOR",
    [TOK_TO] = "TO",
    [TOK_STEP] = "STEP",
    [TOK_NEXT] = "NEXT",
    [TOK_FUNCTION] = "FUNCTION",
    [TOK_RETURNS] = "RETURNS",
    [TOK_ENDFUNCTION] = "ENDFUNCTION",
    [TOK_PROCEDURE] = "PROCEDURE",
    [TOK_ENDPROCEDURE] = "ENDPROCEDURE",
    [TOK_RETURN] = "RETURN",
    [TOK_INCLUDE] = "INCLUDE",
    [TOK_EXPORT] = "EXPORT",
    [TOK_BREAK] = "BREAK",
    [TOK_CONTINUE] = "CONTINUE",
    [TOK_REPEAT] = "REPEAT",
    [TOK_UNTIL] = "UNTIL",
    [TOK_STRUCT] = "STRUCT",
    [TOK_ENDSTRUCT] = "ENDSTRUCT",
    [TOK_INTEGER] = "INTEGER",
    [TOK_REAL] = "REAL",
    [TOK_BOOLEAN] = "BOOLEAN",
    [TOK_STRING] = "STRING",
    [TOK_CHAR] = "CHAR",
    [TOK_NULL] = "NULL",
    [TOK_LPAREN] = "lparen",
    [TOK_RPAREN] = "rparen",
    [TOK_LBRACKET] = "lbracket",
    [TOK_RBRACKET] = "rbracket",
    [TOK_LCURLY] = "lcurly",
    [TOK_RCURLY] = "rcurly",
    [TOK_COMMA] = "comma",
    [TOK_COLON] = "colon",
    [TOK_SEMICOLON] = "semicolon",
    [TOK_DOT] = "dot",
    [TOK_ADD] = "add",
    [TOK_SUB] = "sub",
    [TOK_MUL] = "mul",
    [TOK_DIV] = "div",
    [TOK_CARET] = "caret",
    [TOK_LT] = "lt",
    [TOK_GT] = "gt",
    [TOK_LEQ] = "leq",
    [TOK_GEQ] = "geq",
    [TOK_EQ] = "eq",
    [TOK_NEQ] = "neq",
    [TOK_ASSIGN] = "assign",
    [TOK_SHR] = "shr",
    [TOK_SHL] = "shl",
    [TOK_BITOR] = "bitor",
    [TOK_BITAND] = "bitand",
    [TOK_BITNOT] = "bitnot",
    [TOK_BITXOR] = "bitxor",
};

Token token_new_ident(const char* str) {
    a_string s = astr(str);
    return (Token){
        .kind = TOK_IDENT,
        .data.string = s,
    };
}

void token_free(Token* t) {
    if (t->kind == TOK_IDENT || t->kind == TOK_LITERAL_STRING ||
        t->kind == TOK_LITERAL_CHAR || t->kind == TOK_LITERAL_NUMBER) {
        as_free(&t->data.string);
    }
}

Token token_dupe(Token* t) {
    Token new = {
        .kind = t->kind,
        .pos = t->pos,
    };
    if (t->kind == TOK_IDENT || t->kind == TOK_LITERAL_STRING ||
        t->kind == TOK_LITERAL_CHAR || t->kind == TOK_LITERAL_NUMBER) {
        new.data.string = as_dupe(&t->data.string);
    } else if (t->kind == TOK_LITERAL_BOOLEAN) {
        new.data.boolean = t->data.boolean;
    }

    return new;
}

const char* token_kind_string(TokenKind k) {
    if ((i32)k < 0 || (i32)k > LENGTH(TOKEN_STRINGS))
        panic("invalid token kind");

    return TOKEN_STRINGS[k];
}

void token_print_long(Token* t) {
    printf("token[%d, %d, %d]: ", t->pos.row, t->pos.col, t->pos.span);

    const a_string* s = &t->data.string;
    switch (t->kind) {
        case TOK_IDENT: {
            printf("(%.*s)", as_fmtp(s));
        } break;
        case TOK_LITERAL_STRING: {
            printf("\"%.*s\"", as_fmtp(s));
        } break;
        case TOK_LITERAL_CHAR: {
            printf("'%.*s'", as_fmtp(s));
        } break;
        case TOK_LITERAL_NUMBER: {
            printf("%.*s", as_fmtp(s));
        } break;
        case TOK_LITERAL_BOOLEAN: {
            if (t->data.boolean)
                printf("<true>");
            else
                printf("<false>");
        } break;
        default: token_print(t);
    }

    putchar('\n');
}

void token_print(Token* t) {
    const char* s = token_kind_string(t->kind);
    printf("<%s>", s);
}

// lexer stuff
#define LX_KWT_STRSZ 64

typedef struct {
    char txt[LX_KWT_STRSZ];
    TokenKind kw;
    UT_hash_handle hh;
} KeywordTable;

KeywordTable* kwt;

static void lx_kwt_setup(void);
static void lx_kwt_add(const char* s, TokenKind k);
static TokenKind lx_kwt_get(const char* s);
static void lx_kwt_free(void);

static void lx_kwt_setup(void) {
    kwt = NULL;

    lx_kwt_add("declare", TOK_DECLARE);
    lx_kwt_add("constant", TOK_CONSTANT);
    lx_kwt_add("output", TOK_OUTPUT);
    lx_kwt_add("print", TOK_PRINT);
    lx_kwt_add("input", TOK_INPUT);
    lx_kwt_add("and", TOK_AND);
    lx_kwt_add("or", TOK_OR);
    lx_kwt_add("not", TOK_NOT);
    lx_kwt_add("if", TOK_IF);
    lx_kwt_add("then", TOK_THEN);
    lx_kwt_add("else", TOK_ELSE);
    lx_kwt_add("endif", TOK_ENDIF);
    lx_kwt_add("case", TOK_CASE);
    lx_kwt_add("of", TOK_OF);
    lx_kwt_add("otherwise", TOK_OTHERWISE);
    lx_kwt_add("endcase", TOK_ENDCASE);
    lx_kwt_add("while", TOK_WHILE);
    lx_kwt_add("do", TOK_DO);
    lx_kwt_add("endwhile", TOK_ENDWHILE);
    lx_kwt_add("for", TOK_FOR);
    lx_kwt_add("to", TOK_TO);
    lx_kwt_add("step", TOK_STEP);
    lx_kwt_add("next", TOK_NEXT);
    lx_kwt_add("function", TOK_FUNCTION);
    lx_kwt_add("returns", TOK_RETURNS);
    lx_kwt_add("endfunction", TOK_ENDFUNCTION);
    lx_kwt_add("procedure", TOK_PROCEDURE);
    lx_kwt_add("endprocedure", TOK_ENDPROCEDURE);
    lx_kwt_add("return", TOK_RETURN);
    lx_kwt_add("include", TOK_INCLUDE);
    lx_kwt_add("export", TOK_EXPORT);
    lx_kwt_add("break", TOK_BREAK);
    lx_kwt_add("continue", TOK_CONTINUE);
    lx_kwt_add("repeat", TOK_REPEAT);
    lx_kwt_add("until", TOK_UNTIL);
    lx_kwt_add("struct", TOK_STRUCT);
    lx_kwt_add("endstruct", TOK_ENDSTRUCT);
    lx_kwt_add("integer", TOK_INTEGER);
    lx_kwt_add("real", TOK_REAL);
    lx_kwt_add("boolean", TOK_BOOLEAN);
    lx_kwt_add("string", TOK_STRING);
    lx_kwt_add("char", TOK_CHAR);
    lx_kwt_add("null", TOK_NULL);
}

static void lx_kwt_add(const char* s, TokenKind k) {
    KeywordTable* row = malloc(sizeof(KeywordTable));
    check_alloc(row);
    strncpy(row->txt, s, LX_KWT_STRSZ);
    row->kw = k;
    HASH_ADD_STR(kwt, txt, row);
}

static TokenKind lx_kwt_get(const char* s) {
    KeywordTable* out;
    HASH_FIND_STR(kwt, s, out);
    if (out)
        return out->kw;
    else
        return TOK_INVALID;
}

static void lx_kwt_free(void) {
    KeywordTable *elem, *tmp;

    HASH_ITER(hh, kwt, elem, tmp) {
        HASH_DEL(kwt, elem);
        free(elem);
    }
}

static void lx_trim_spaces(Lexer* l);
static void lx_trim_comment(Lexer* l);
static bool lx_is_separator(char ch);
static bool lx_is_operator_start(char ch);

static bool lx_next_double_symbol(Lexer* l); // true if found
static bool lx_next_single_symbol(Lexer* l); // true if found
static bool lx_next_word(Lexer* l, a_string* res);
static bool lx_next_keyword(Lexer* l, const a_string* word);
static bool lx_next_literal(Lexer* l, const a_string* word);

static bool lx_is_separator(char ch) {
    return strchr("{}[]();:,.", ch);
}

static bool lx_is_operator_start(char ch) {
    return strchr("+-*/=<>&|^~", ch);
}

Lexer lx_new(const char* src, usize src_len) {
    lx_kwt_setup();
    Lexer res = {.src = src, .src_len = src_len, .row = 1};
    return res;
}

static void lx_trim_spaces(Lexer* l) {
    if (!IN_BOUNDS) {
        return;
    }

    while (l->cur < l->src_len && isspace(CUR) && CUR != '\n')
        l->cur++;

    lx_trim_comment(l);
    return;
}

static void lx_trim_comment(Lexer* l) {
    if (l->cur + 2 >= l->src_len) {
        return;
    }

    if (!strncmp(&CUR, "//", 2)) {
        l->cur += 2; // skip past comment marker

        while (IN_BOUNDS && CUR != '\n')
            l->cur++;

        lx_trim_spaces(l);
        return;
    }

    if (!strncmp(&CUR, "/*", 2)) {
        l->cur += 2; // skip past

        while (IN_BOUNDS && strncmp(&CUR, "*/", 2)) {
            if (CUR == '\n')
                BUMP_NEWLINE;
            else
                l->cur++;
        }

        // we found */
        l->cur += 2;

        lx_trim_spaces(l);
        return;
    }
}

static bool lx_next_double_symbol(Lexer* l) {
    if (!lx_is_operator_start(CUR))
        return false;

    if (l->cur + 1 >= l->src_len) {
        return false;
    }

    if (!strncmp(&CUR, "<-", 2)) {
        l->token = TOK(ASSIGN, 2);
    } else if (!strncmp(&CUR, ">=", 2)) {
        l->token = TOK(GEQ, 2);
    } else if (!strncmp(&CUR, "<=", 2)) {
        l->token = TOK(LEQ, 2);
    } else if (!strncmp(&CUR, "<>", 2)) {
        l->token = TOK(NEQ, 2);
    } else if (!strncmp(&CUR, ">>", 2)) {
        l->token = TOK(SHR, 2);
    } else if (!strncmp(&CUR, "<<", 2)) {
        l->token = TOK(SHL, 2);
    } else if (!strncmp(&CUR, "~^", 2)) {
        l->token = TOK(BITXOR, 2);
    } else {
        return false;
    }

    l->token.pos = POS_HERE(2);
    l->cur += 2;

    return true;
}

static bool lx_next_single_symbol(Lexer* l) {
    if (!lx_is_separator(CUR) && !lx_is_operator_start(CUR))
        return false;

    const TokenKind TABLE[] = {
        ['{'] = TOK_LCURLY,   ['}'] = TOK_RCURLY, ['['] = TOK_LBRACKET,
        [']'] = TOK_RBRACKET, ['('] = TOK_LPAREN, [')'] = TOK_RPAREN,
        [':'] = TOK_COLON,    [','] = TOK_COMMA,  [';'] = TOK_SEMICOLON,
        ['>'] = TOK_LT,       ['<'] = TOK_GT,     ['='] = TOK_EQ,
        ['*'] = TOK_MUL,      ['/'] = TOK_DIV,    ['+'] = TOK_ADD,
        ['-'] = TOK_SUB,      ['^'] = TOK_CARET,  ['&'] = TOK_BITAND,
        ['|'] = TOK_BITOR,    ['.'] = TOK_DOT,    ['~'] = TOK_BITNOT,
    };

    TokenKind t;
    if ((t = TABLE[(int)CUR]) == 0)
        return false;

    l->token = (Token){
        .kind = t,
        .pos = POS_HERE(1),
    };

    l->cur++;

    return true;
}

static bool lx_next_word(Lexer* l, a_string* res) {
    const char* begin = &CUR;
    u32 len = 0;
    const char DELIMS[] = "\"'";
    bool delimited_literal = strchr(DELIMS, *begin);
    char delim = 0;
    if (delimited_literal) {
        delim = CUR;
        l->cur++;
        len++;
    }

    do {
        bool stop;

        if (!IN_BOUNDS)
            break;

        if (delimited_literal)
            stop = CUR == delim;
        else
            stop = lx_is_operator_start(CUR) || lx_is_separator(CUR) ||
                   isspace(CUR) || strchr(DELIMS, CUR);

        if (CUR == '\\') {
            len++;
            l->cur++;
        }

        if (stop)
            break;

        len++;
        l->cur++;
    } while (1);

    if (delimited_literal) {
        if (!IN_BOUNDS) {
            l->error = ERROR(EOF, 1);
            return false;
        }

        if (CUR != delim) {
            if (strchr(DELIMS, CUR) != NULL)
                l->error = ERROR(MISMATCHED_DELIMITER, len);
            else
                l->error = ERROR(UNTERMINATED_LITERAL, len);
            return false;
        } else {
            l->cur++;
            len++;
        }
    }

    *res = as_new();
    as_ncopy_cstr(res, begin, len);

    return true;
}

static bool lx_next_keyword(Lexer* l, const a_string* word) {
    if (word->len >= LX_KWT_STRSZ)
        return false;

    TokenKind kw;

    if (!as_is_case_consistent(word))
        return false;

    a_string word_lower = as_tolower(word);
    if ((kw = lx_kwt_get(word_lower.data)) != TOK_INVALID) {
        l->token = TOKEN(kw, word_lower.len);
        as_free(&word_lower);
        return true;
    } else {
        as_free(&word_lower);
        return false;
    }
}

static bool is_number(const a_string* word) {
    // edge case: single decimal
    if (as_first(word) == '.' && word->len == 1)
        return false;

    bool found_decimal = false;
    for (usize i = 0; i < word->len; i++) {
        char cur = as_at(word, i);

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

    return true;
}

static bool lx_next_literal(Lexer* l, const a_string* word) {
    char* p;

    if ((p = strchr("\"'", as_first(word)))) {
        if (word->len == 1)
            unreachable;

        a_string res = as_slice(word, 1, word->len - 1);
        TokenKind k = (*p == '\'') ? TOK_LITERAL_CHAR : TOK_LITERAL_STRING;

        l->token = (Token){
            .kind = k,
            .pos = POS(word->len),
            .data.string = res,
        };
        return true;
    }

    if (is_number(word)) {
        a_string new = as_dupe(word);
        if (!as_valid(&new))
            panic("failed to dupe a_string for number");

        l->token = (Token){
            .kind = TOK_LITERAL_NUMBER,
            .pos = POS(word->len),
            .data.string = new,
        };
        return true;
    }

    if (as_is_case_consistent(word)) {
        if (as_equal_case_insensitive_cstr(word, "true")) {
            l->token = (Token){
                .kind = TOK_LITERAL_BOOLEAN,
                .pos = POS(word->len),
                .data.boolean = true,
            };
            return true;
        } else if (as_equal_case_insensitive_cstr(word, "false")) {
            l->token = (Token){
                .kind = TOK_LITERAL_BOOLEAN,
                .pos = POS(word->len),
                .data.boolean = false,
            };
            return true;
        }
    }

    return false;
}

static bool is_ident(const a_string* s) {
    char first = as_first(s);
    if (!isalpha(first) && first != '_')
        return false;

    for (size_t i = 0; i < s->len; i++) {
        char ch = as_at(s, i);
        if (!isalnum(ch) && !strchr("_.", ch))
            return false;
    }

    return true;
}

static bool lx_next_ident(Lexer* l, const a_string* word) {
    if (is_ident(word)) {
        a_string new = as_dupe(word);
        if (!as_valid(&new))
            panic("failed to dupe a_string for ident");

        l->token = (Token){
            .kind = TOK_IDENT,
            .data.string = new,
            .pos = POS(new.len),
        };
        return true;
    } else {
        l->error = ERROR(INVALID_IDENTIFIER, word->len);
        return false;
    }
}

Token* lx_next_token(Lexer* l) {
    if (l->error.kind != LX_ERROR_NULL) {
        token_free(&l->token);
    }

    l->token = (Token){0};
    l->error = (LexerError){0};

    lx_trim_spaces(l);
    if (l->cur >= l->src_len) {
        l->token = TOK(EOF, 1);
        goto done;
    }

    if (CUR == '\n') {
        l->token = TOK(NEWLINE, 1);
        BUMP_NEWLINE;
        goto done;
    }

    TRY(lx_next_double_symbol(l));
    TRY(lx_next_single_symbol(l));

    a_string word = {0};
    if (!lx_next_word(l, &word)) // error
        return NULL;

    TRY_AND_FREE(lx_next_keyword(l, &word));
    TRY_AND_FREE(lx_next_literal(l, &word));
    TRY_AND_FREE(lx_next_ident(l, &word));

next_token_error:
    as_free(&word);
done:
    return &l->token;
}

bool lx_tokenize(Lexer* l, Tokens* out) {
    *out = (Tokens){0};
    Token* tok = {0};

    do {
        tok = lx_next_token(l);

        if (!tok) {
            lx_perror(l->error.kind, "\033[31;1mlexer error\033[0m");
            return false;
        }

        av_append(out, *tok);
    } while (!tok || tok->kind != TOK_EOF);

    return true;
}

void lx_free(Lexer* l) {
    (void)l;
    lx_kwt_free();
}

static const char* LEXER_ERROR_TABLE[] = {
    [LX_ERROR_NULL] = "(no error)",
    [LX_ERROR_UNTERMINATED_LITERAL] =
        "unterminated string or character literal",
    [LX_ERROR_EOF] = "unexpected end of file",
    [LX_ERROR_BAD_ESCAPE] = "bad escape sequence",
    [LX_ERROR_MISMATCHED_DELIMITER] =
        "mismatched delimiter in delimited literal",
    [LX_ERROR_INVALID_IDENTIFIER] = "invalid identifier",
    [LX_ERROR_CHAR_LITERAL_TOO_LONG] = "character literal is too long",
};

const char* lx_strerror(LexerErrorKind k) {
    return LEXER_ERROR_TABLE[k];
}

a_string lx_as_strerror(LexerErrorKind k) {
    return astr(lx_strerror(k));
}

void lx_perror(LexerErrorKind k, const char* pre) {
    const char* err = lx_strerror(k);
    eprintf("%s: %s\n", pre, err);
}

void lx_reset(Lexer* l) {
    l->row = 1;
    l->bol = 0;
    l->cur = 0;
}

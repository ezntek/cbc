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

#include "lexertypes.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "a_string.h"
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"

Parser parser_new(Tokens toks);
void parser_free(Parser* ps);

typedef struct {
    Token* data;
    bool have;
} MaybeToken;
#define HAVE_TOKEN(t)                                                          \
    (MaybeToken) {                                                             \
        .data = (t), .have = true                                              \
    }
#define NO_TOKEN (MaybeToken){0}

// utility functions
static MaybeToken ps_peek(Parser* ps);
static MaybeToken ps_prev(Parser* ps);
static MaybeToken ps_peek_next(Parser* ps);
static MaybeToken ps_get(Parser* ps, usize idx);
static MaybeToken ps_peek_and_expect(Parser* ps, TokenKind expected);
static bool ps_check(Parser* ps, TokenKind expected);
static MaybeToken ps_check_and_consume(Parser* ps, TokenKind expected);
static MaybeToken ps_consume_and_expect(Parser* ps, TokenKind expected);
static Pos ps_get_pos(Parser* ps);
static bool ps_bump_error_count(Parser* ps);
static void ps_diag(Parser* ps, const char* format, ...);
static void ps_diag_expected(Parser* ps, const char* thing);

static MaybeToken ps_consume(Parser* ps) {
    if (ps->cur < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur++]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

static MaybeToken ps_peek(Parser* ps) {
    if (ps->cur < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

static MaybeToken ps_peek_next(Parser* ps) {
    if (ps->cur + 1 < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur + 1]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

static MaybeToken ps_prev(Parser* ps) {
    if (ps->cur - 1 < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[ps->cur - 1]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

static MaybeToken ps_get(Parser* ps, usize idx) {
    if (idx < ps->tokens_len) {
        return HAVE_TOKEN(&ps->tokens[idx]);
    } else {
        ps->eof = true;
        return NO_TOKEN;
    }
}

static MaybeToken ps_peek_and_expect(Parser* ps, TokenKind expected) {
    const char* expected_s = token_kind_string(expected);
    if_let(Token*, t, ps_peek(ps)) {
        if (t->kind == TOK_EOF) {
            ps_diag(ps, "expected token %s, but reached end of file",
                    token_kind_string(expected));
        } else if (t->kind != expected) {
            ps_diag(ps, "expected token %s, but found %s",
                    token_kind_string(t->kind), expected_s);
        } else {
            return HAVE_TOKEN(t);
        }
        return NO_TOKEN;
    }

    ps_diag(ps, "expected token %s, but got no token", expected_s);
    return NO_TOKEN;
}

static bool ps_check(Parser* ps, TokenKind expected) {
    if_let(Token*, t, ps_peek(ps)) {
        return t->kind == expected;
    }
    return false;
}

static MaybeToken ps_check_and_consume(Parser* ps, TokenKind expected) {
    if (ps_check(ps, expected)) {
        return ps_consume(ps);
    } else {
        return NO_TOKEN;
    }
}

static MaybeToken ps_consume_and_expect(Parser* ps, TokenKind expected) {
    const char* expected_s = token_kind_string(expected);
    if_let(Token*, t, ps_consume(ps)) {
        if (t->kind != expected) {
            ps_diag(ps, "expected token \"%s\" but got \"%s\"", expected_s,
                    token_kind_string(t->kind));
        } else {
            return HAVE_TOKEN(t);
        }
        return NO_TOKEN;
    }

    ps_diag(ps, "expected token \"%s\" but reached the end of the token stream",
            expected_s);
    return NO_TOKEN;
}

static Pos ps_get_pos(Parser* ps) {
    if_let(Token*, t, ps_prev(ps)) {
        return t->pos;
    }

    panic("no previous token");
}

void ps_diag_at(Parser* ps, Pos pos, const char* format, ...) {
    eprintf("\033[31;1merror: \033[0;1m%.*s:%u:%u: \033[0m",
            (int)ps->file_name.len, ps->file_name.data, pos.row, pos.col);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    putchar('\n');
    ps->error_reported = true;
}

void ps_diag(Parser* ps, const char* format, ...) {
    Pos pos = ps_get_pos(ps);
    // FIXME: less code duplication due to va_list
    eprintf("\033[31;1merror: \033[0;1m%.*s:%u:%u: \033[0m",
            (int)ps->file_name.len, ps->file_name.data, pos.row, pos.col);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    putchar('\n');
    ps->error_reported = true;
}

static bool ps_bump_error_count(Parser* ps) {
    return ++ps->cur > MAX_ERROR_COUNT;
}

static void ps_diag_expected(Parser* ps, const char* thing) {
    if_let(Token*, tok, ps_peek(ps)) {
        ps_diag(ps, "expected %s, but found token \"%s\"",
                token_kind_string(tok->kind));
        return;
    }

    ps_diag(ps, "expected %s, but found no token", thing);
}

#define diag_token(t, ...) ps_diag_at(ps, (t)->pos, __VA_ARGS__)

// actual parsing functions
static bool ps_literal(Parser* ps);
static bool ps_primary(Parser* ps);
static bool ps_postfix(Parser* ps);
static bool ps_unary(Parser* ps);
static bool ps_binary(Parser* ps);

static char resolve_escape(char ch) {
    switch (ch) {
        case 'a': {
            return '\a';
        } break;
        case 'b': {
            return '\b';
        } break;
        case 'e': {
            return '\033';
        } break;
        case 'n': {
            return '\n';
        } break;
        case 'r': {
            return '\r';
        } break;
        case 't': {
            return '\t';
        } break;
        case '\\': {
            return '\\';
        } break;
        case '\'': {
            return '\'';
        } break;
        case '"': {
            return '\"';
        } break;
        default: return -1;
    }
}

static bool is_int(a_string* s) {
    if (!isdigit(as_first(s)))
        return false;

    char ch;
    for (usize i = 1; i < s->len; ++i) {
        ch = as_at(s, i);
        if (!isdigit(ch) && ch != '_')
            return false;
    }

    return true;
}

static bool is_real(a_string* s) {
    if (!isdigit(as_first(s)) && as_first(s) != '.')
        return false;

    if (is_int(s))
        return false;

    bool found = false;
    char ch;
    // starting from 0 because the first item might be a '.'
    for (usize i = 0; i < s->len; ++i) {
        ch = as_at(s, i);
        if (ch == '.') {
            if (found)
                return false;
            found = true;
            continue;
        }

        if (!isdigit(ch) && ch != '_')
            return false;
    }

    return true;
}

static bool is_literal(TokenKind k) {
    switch (k) {
        case TOK_NULL:
        case TOK_LITERAL_NUMBER:
        case TOK_LITERAL_BOOLEAN:
        case TOK_LITERAL_CHAR:
        case TOK_LITERAL_STRING: return true;
        default: return false;
    }
}

static bool ps_literal(Parser* ps) {
    MaybeToken peek = ps_peek(ps);
    if (peek.have && !is_literal(peek.data->kind)) {
        return false;
    }

    let_else(Token*, t, ps_consume(ps)) {
        ps_diag_at(ps, t->pos, "failed to get next literal");
        return false;
    }

    Token* t = ps_prev(ps).data;
    a_string* s = &t->data.string;

    switch (t->kind) {
        case TOK_NULL: {
            ps->expr = cb_expr_new_literal(t->pos, cb_value_new_null());
            return true;
        } break;
        case TOK_LITERAL_NUMBER: {
            if (is_real(s)) {
                double res;
                usize eidx = 0;
                if ((eidx = as_to_double(s, &res)) != s->len) {
                    if (errno == ERANGE) {
                        diag_token(t,
                                   "float literal \"%.*s\" is either too large "
                                   "or too small!",
                                   as_fmtp(s));
                        return false;
                    }

                    Pos p = t->pos;
                    p.col += eidx;
                    p.span -= eidx;
                    ps_diag_at(ps, p, "float literal \"%.*s\" is invalid!",
                               as_fmtp(s));
                    return false;
                }
                ps->expr = cb_expr_new_literal(t->pos, cb_value_new_real(res));
                return true;
            } else if (is_int(s)) {
                int64_t res;
                usize eidx = 0;
                if ((eidx = as_to_integer(s, &res, 0)) != s->len) {
                    if (errno == ERANGE) {
                        ps_diag_at(ps, t->pos,
                                   "int literal \"%s\" is either too large or "
                                   "too small!",
                                   s->data);
                        return false;
                    } else {
                        Pos p = t->pos;
                        p.col += eidx;
                        p.span -= eidx;
                        ps_diag_at(ps, p, "int literal \"%s\" is invalid!",
                                   s->data);
                        return false;
                    }
                }
                ps->expr =
                    cb_expr_new_literal(t->pos, cb_value_new_integer(res));
                return true;
            } else {
                diag_token(t, "found invalid number literal \"%.*s\"",
                           as_fmtp(s));
                return false;
            }
        } break;
        case TOK_LITERAL_BOOLEAN: {
            ps->expr = cb_expr_new_literal(
                t->pos, cb_value_new_boolean(t->data.boolean));
            return true;
        } break;
        case TOK_LITERAL_STRING: {
            a_string res = as_with_capacity(s->len + 1);
            char ch;
            for (usize i = 0; i < s->len; i++) {
                ch = as_at(s, i);

                if (ch == '\\') {
                    if (i == s->len - 1) {
                        diag_token(
                            t, "last character of string literal is an escape");
                        as_free(&res);
                        return false;
                    }

                    ch = resolve_escape(as_at(s, ++i));
                    if (ch == -1) {
                        diag_token(t,
                                   "invalid escape sequence in string literal");
                        as_free(&res);
                        return false;
                    }
                }

                as_append_char(&res, ch);
            }
            ps->expr = cb_expr_new_literal(t->pos, cb_value_new_string(res));
            as_free(&res);
            return true;
        } break;
        case TOK_LITERAL_CHAR: {
            if (s->len == 0) {
                diag_token(t, "not enough characters in character literal");
                return false;
            }

            char ch;
            if (as_at(s, 0) == '\\') {
                if (s->len == 1) {
                    diag_token(t, "invalid escape in character literal");
                    return false;
                }

                if ((ch = resolve_escape(as_at(s, 1))) == -1) {
                    diag_token(t, "invalid escape in character literal");
                    return false;
                }
            } else if (s->len >= 2) {
                diag_token(t, "character literal is too long!");
                return false;
            } else {
                ch = as_at(s, 0);
            }

            ps->expr = cb_expr_new_literal(t->pos, cb_value_new_char(ch));
            return true;
        } break;
        default: return false;
    }

    return false;
}

static bool ps_primary(Parser* ps) {
    if (ps_check(ps, TOK_LPAREN)) {
        Token* t = ps_consume(ps).data;
        if (!ps_expr(ps)) {
            diag_token(t, "invalid expression inside grouping");
        }
        ps->expr = cb_expr_new_unary(t->pos, CB_EXPR_GROUPING, ps->expr);
        if (!ps_consume_and_expect(ps, TOK_RPAREN).have) {
            return false; // XXX: are we sure we reported it?
        }
        return true;
    }

    if (ps_literal(ps)) {
        return true;
    } else {
        if (ps->error_reported)
            return false;
    }

    if (ps_check(ps, TOK_IDENT)) {
        Token* p = ps_consume(ps).data;
        ps->expr = cb_expr_new_ident(p->pos, p->data.string);
        return true;
    }

    return false;
}

static bool ps_postfix(Parser* ps) {
    MaybeToken peek = ps_peek(ps);
    if (!peek.have) {
        return false;
    }

    if (!ps_primary(ps)) {
        ps_diag_at(ps, peek.data->pos, "could not parse primary expression");
        return false;
    }

    do {
        if (ps->eof)
            break;

        if_let(Token*, t, ps_peek(ps)) {
            switch (t->kind) {
                case TOK_CARET: {
                    (void)ps_consume(ps);
                    ps->expr = cb_expr_new_unary(ps_get_pos(ps), CB_EXPR_DEREF,
                                                 ps->expr);
                } break;
                case TOK_LPAREN: {
                    // TODO: function call
                } break;
                case TOK_LBRACKET: {
                    // TODO: array index
                } break;
                case TOK_DOT: {
                    // TODO: array index
                } break;
                default: goto done;
            }
        }
    } while (1);
done:

    return true;
}

static const CB_ExprKind UNARY_OP_TABLE[] = {
    [TOK_SUB] = CB_EXPR_NEGATION,
    [TOK_CARET] = CB_EXPR_REF,
    [TOK_NOT] = CB_EXPR_NOT,
    [TOK_BITNOT] = CB_EXPR_BITNOT,
};

static bool ps_unary(Parser* ps) {
    if_let(Token*, t, ps_peek(ps)) {
        // prefix ops
        switch (t->kind) {
            case TOK_SUB:
            case TOK_CARET:
            case TOK_NOT:
            case TOK_BITNOT: {
                Pos p = ps_consume(ps).data->pos;
                if (!ps_unary(ps)) {
                    ps_diag(ps, "could not parse expression after %s",
                            token_kind_string(t->kind));
                    return false;
                }
                ps->expr =
                    cb_expr_new_unary(p, UNARY_OP_TABLE[t->kind], ps->expr);
            } break;
            default: break;
        }
        // if we still have tokens to postfix
        if (!ps->eof)
            return ps_postfix(ps);
        else // assume everything OK
            return true;
    }
    // we probably hit EOF
    return false;
}

static bool ps_binary(Parser* ps) {
    return ps_unary(ps);
}

bool ps_expr(Parser* ps) {
    return ps_binary(ps);
}

bool ps_stmt(Parser* ps) {
    return false;
}

Parser ps_new(Token* tokens, usize tokens_len, a_string file_name) {
    if (tokens_len < 1)
        panic("invalid token length (missing EOF token)");

    return (Parser){
        .tokens = tokens, .tokens_len = tokens_len - 1, .file_name = file_name};
}

void ps_free(Parser* ps) {
    as_free(&ps->file_name);
}

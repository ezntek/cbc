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
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>

#include "../a_string.h"
#include "../ast.h"
#include "../common.h"
#include "../lexer.h"
#include "../lexer_types.h"
#include "parser.h"
#include "parser_internal.h"

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

bool is_int(a_string* s) {
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

bool is_real(a_string* s) {
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

bool is_literal(TokenKind k) {
    switch (k) {
        case TOK_NULL:
        case TOK_LITERAL_NUMBER:
        case TOK_LITERAL_BOOLEAN:
        case TOK_LITERAL_CHAR:
        case TOK_LITERAL_STRING: return true;
        default: return false;
    }
}

bool parse_literal(Parser* ps) {
    Token* peek = ps_peek(ps);
    Token* t = NULL;

    if (peek && !is_literal(peek->kind)) {
        return false;
    }

    if (!(t = ps_consume(ps))) {
        ps_diag_at(ps, t->pos, "failed to get next literal");
        return false;
    }

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

bool parse_primary(Parser* ps) {
    if (ps_check(ps, TOK_LPAREN)) {
        Token* t = ps_consume(ps);
        if (!ps_expr(ps)) {
            diag_token(t, "invalid expression inside grouping");
        }
        ps->expr = cb_expr_new_unary(t->pos, CB_EXPR_GROUPING, ps->expr);
        if (!ps_consume_and_expect(ps, TOK_RPAREN)) {
            return false; // XXX: are we sure we reported it?
        }
        return true;
    }

    if (parse_literal(ps)) {
        return true;
    } else {
        if (ps->error_count)
            return false;
    }

    if (ps_check(ps, TOK_IDENT)) {
        Token* p = ps_consume(ps);
        ps->expr = cb_expr_new_ident(p->pos, p->data.string);
        return true;
    }

    return false;
}

bool can_start_primary_unary(TokenKind k) {
    // not a
    // ^a
    // ~a
    // -a
    switch (k) {
        // TODO: array literals
        case TOK_LITERAL_BOOLEAN:
        case TOK_LITERAL_STRING:
        case TOK_LITERAL_CHAR:
        case TOK_LITERAL_NUMBER:
        case TOK_IDENT:
        // possible unaries
        case TOK_LPAREN:
        case TOK_NOT:
        // case TOK_CARET:
        case TOK_BITNOT:
        case TOK_SUB: return true;
        default: return false;
    }
}

bool parse_postfix(Parser* ps) {
    Token* peek = ps_peek(ps);
    if (!peek) {
        return false;
    }

    if (!parse_primary(ps)) {
        ps_diag_at(ps, peek->pos, "could not parse primary expression");
        return false;
    }

    do {
        if (ps->eof)
            break;

        Token* t = ps_peek(ps);
        if (!t)
            break;

        switch (t->kind) {
            case TOK_CARET: {
                Token* next = ps_peek_next(ps);
                if (next && can_start_primary_unary(next->kind)) {
                    // infix, ignore
                    goto done;
                } else {
                    (void)ps_consume(ps);
                    ps->expr = cb_expr_new_unary(ps_get_pos(ps), CB_EXPR_DEREF,
                                                 ps->expr);
                }
            } break;
            case TOK_LPAREN: {
                // TODO: function call
            } break;
            case TOK_LBRACKET: {
                // TODO: array index
            } break;
            case TOK_DOT: {
                // TODO: struct access
            } break;
            default: goto done;
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

bool parse_unary(Parser* ps) {
    Token* t = ps_peek(ps);
    if (!t)
        return false;

    // prefix ops
    switch (t->kind) {
        case TOK_SUB:
        case TOK_CARET:
        case TOK_NOT:
        case TOK_BITNOT: {
            Pos p = ps_consume(ps)->pos;
            if (!parse_unary(ps)) {
                ps_diag(ps, "could not parse expression after %s",
                        token_kind_string(t->kind));
                return false;
            }
            ps->expr = cb_expr_new_unary(p, UNARY_OP_TABLE[t->kind], ps->expr);
        } break;
        default: break;
    }

    // if we still have tokens to postfix
    if (!ps->eof && can_start_primary_unary(ps_peek(ps)->kind))
        return parse_postfix(ps);
    else // assume everything OK
        return true;

    // we probably hit EOF
    return false;
}

static const u8 PRECS[] = {
    [TOK_CARET] = 12, [TOK_MUL] = 11,  [TOK_DIV] = 11,   [TOK_ADD] = 10,
    [TOK_SUB] = 10,   [TOK_SHR] = 9,   [TOK_SHL] = 9,    [TOK_LT] = 8,
    [TOK_GT] = 8,     [TOK_LEQ] = 8,   [TOK_GEQ] = 8,    [TOK_EQ] = 7,
    [TOK_NEQ] = 7,    [TOK_BITOR] = 6, [TOK_BITAND] = 5, [TOK_BITXOR] = 4,
    [TOK_AND] = 3,    [TOK_OR] = 2,
};

static const CB_ExprKind BINARY_OP_TABLE[] = {
    [TOK_ADD] = CB_EXPR_ADD,       [TOK_SUB] = CB_EXPR_SUB,
    [TOK_MUL] = CB_EXPR_MUL,       [TOK_DIV] = CB_EXPR_DIV,
    [TOK_CARET] = CB_EXPR_POW,     [TOK_LT] = CB_EXPR_LT,
    [TOK_GT] = CB_EXPR_GT,         [TOK_LEQ] = CB_EXPR_LEQ,
    [TOK_GEQ] = CB_EXPR_GEQ,       [TOK_EQ] = CB_EXPR_EQ,
    [TOK_NEQ] = CB_EXPR_NEQ,       [TOK_SHL] = CB_EXPR_SHL,
    [TOK_SHR] = CB_EXPR_SHR,       [TOK_OR] = CB_EXPR_OR,
    [TOK_AND] = CB_EXPR_AND,       [TOK_BITOR] = CB_EXPR_BITOR,
    [TOK_BITAND] = CB_EXPR_BITAND, [TOK_BITXOR] = CB_EXPR_BITXOR,
};

#define right_assoc(k) ((k) == TOK_CARET)

// min_prec = 0 on initial calls
bool parse_binary(Parser* ps, u8 min_prec) {
    if (!parse_unary(ps))
        return false;

    CB_Expr left = ps->expr;
    Token* peek = {0};
    TokenKind kind = {0};
    u8 cur_prec = 0;

    while ((peek = ps_peek(ps)) &&
           (cur_prec = PRECS[(kind = peek->kind)]) != 0 &&
           PRECS[kind] >= min_prec) {

        (void)ps_consume(ps);
        u8 right_prec = right_assoc(kind) ? cur_prec : cur_prec + 1;
        if (!parse_binary(ps, right_prec))
            return false;

        // the newly parsed rhs sohuld be in ps->expr
        left = cb_expr_new_binary(peek->pos, BINARY_OP_TABLE[kind], left,
                                  ps->expr);
    }

    ps->expr = left;
    return true;
}

bool ps_expr(Parser* ps) {
    return parse_binary(ps, 0);
}

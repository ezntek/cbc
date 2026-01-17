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

#include <stdarg.h>
#include <stdlib.h> // used in macro

#include "../a_string.h"
#include "../a_vector.h"
#include "../ast.h"
#include "../common.h"
#include "compiler.h"
#include "compiler_internal.h"

static Val cm_literal(Compiler* c, CB_Value* e) {
    usize id = c->id++;

    switch (e->kind) {
        case CB_PRIM_STRING: {
            av_append(&c->ss, as_slice_cstr(e->string.data, 0, e->string.len));
            cm_writefln(c, "%%r%zu =l copy $__S%zu", id, c->string_id++);
            return val(id, CB_PRIM_STRING);
        } break;
        case CB_PRIM_INTEGER: {
            cm_writefln(c, "%%r%zu =l copy %li", id, e->integer);
            return val(id, CB_PRIM_INTEGER);
        } break;
        case CB_PRIM_REAL: {
            cm_writefln(c, "%%r%zu =d copy d_%g", id, e->real);
            return val(id, CB_PRIM_REAL);
        } break;
        case CB_PRIM_BOOLEAN: {
            cm_writefln(c, "%%r%zu =w copy %d", id, e->boolean);
            return val(id, CB_PRIM_BOOLEAN);
        } break;
        case CB_PRIM_CHAR: {
            cm_writefln(c, "%%r%zu =w copy %d", id, e->chr);
            return val(id, CB_PRIM_CHAR);
        } break;
        default: {
        } break;
            /*
            case CB_PRIM_NULL: {

            } break;
            */
    }

    panic("not implemented");
}

Val cm_unary(Compiler* c, CB_Expr* e) {
    Val inner = cm_expr(c, e->unary);
    if (!inner.have)
        goto fail;

    Pos pos = e->unary->pos;
    usize id;
    if (e->kind != CB_EXPR_GROUPING)
        id = c->id++;

    switch (e->kind) {
        case CB_EXPR_NEGATION: {
            if (inner.kind != CB_PRIM_INTEGER && inner.kind != CB_PRIM_REAL) {
                cm_diag(c, pos, "cannot negate a value of type %s",
                        type_string(inner.kind));
                goto fail;
            }

            if (inner.kind == CB_PRIM_REAL)
                cm_writefln(c, "%%r%zu =d neg %%r%zu", id, inner.id);
            else if (inner.kind == CB_PRIM_INTEGER)
                cm_writefln(c, "%%r%zu =l neg %%r%zu", id, inner.id);
        } break;
        case CB_EXPR_NOT: {
            if (inner.kind != CB_PRIM_BOOLEAN) {
                cm_diag(c, pos, "cannot do boolean NOT on a value of type %s",
                        type_string(inner.kind));
                goto fail;
            }

            cm_writefln(c, "%%r%zu =w ceqw %%r%zu, 0", id, inner.id);
        } break;
        case CB_EXPR_BITNOT: {
            if (inner.kind == CB_PRIM_STRING || inner.kind > CB_PRIM_CUSTOM) {
                cm_diag(c, pos, "cannot do bitwise NOT on a value of type %s",
                        type_string(inner.kind));
                goto fail;
            }

            cm_writefln(c, "%%r%zu =l xor %%r%zu, 18446744073709551615", id,
                        inner.id);
        } break;
        case CB_EXPR_GROUPING: {
            // we don't actually need anything
        } break;
        case CB_EXPR_TYPECAST: {
            panic("not implemented");
        } break;
        case CB_EXPR_FNCALL: {
            panic("not implemented");
        } break;
        case CB_EXPR_REF: {
            panic("not implemented");
        } break;
        case CB_EXPR_DEREF: {
            panic("not implemented");
        } break;
        default: panic("tried to compile a non-unary expr as a unary");
    }

    return val(id, inner.kind);
fail:
    return (Val){0};
}

Val cm_binary(Compiler* c, CB_Expr* e) {
    Val lhs_v = cm_expr(c, e->unary), rhs_v = cm_expr(c, e->unary);
    Pos lhs_pos = e->lhs->pos, rhs_pos = e->rhs->pos;

    if (!lhs_v.have)
        goto fail;

    if (!rhs_v.have)
        goto fail;

    switch (e->kind) {
        case CB_EXPR_ADD: {
            panic("not implemented");
        } break;
        case CB_EXPR_SUB: {
            panic("not implemented");
        } break;
        case CB_EXPR_MUL: {
            panic("not implemented");
        } break;
        case CB_EXPR_DIV: {
            panic("not implemented");
        } break;
        case CB_EXPR_POW: {
            panic("not implemented");
        } break;
        case CB_EXPR_LT: {
            panic("not implemented");
        } break;
        case CB_EXPR_GT: {
            panic("not implemented");
        } break;
        case CB_EXPR_LEQ: {
            panic("not implemented");
        } break;
        case CB_EXPR_GEQ: {
            panic("not implemented");
        } break;
        case CB_EXPR_EQ: {
            panic("not implemented");
        } break;
        case CB_EXPR_NEQ: {
            panic("not implemented");
        } break;
        case CB_EXPR_SHL: {
            panic("not implemented");
        } break;
        case CB_EXPR_SHR: {
            panic("not implemented");
        } break;
        case CB_EXPR_OR: {
            panic("not implemented");
        } break;
        case CB_EXPR_AND: {
            panic("not implemented");
        } break;
        case CB_EXPR_BITOR: {
            panic("not implemented");
        } break;
        case CB_EXPR_BITAND: {
            panic("not implemented");
        } break;
        case CB_EXPR_BITXOR: {
            panic("not implemented");
        } break;
    }

fail:
    return (Val){0};
}

Val cm_expr(Compiler* c, CB_Expr* e) {
    if (cb_expr_kind_is_unary(e->kind)) {
        return cm_unary(c, e);
    } else if (cb_expr_kind_is_binary(e->kind)) {
        return cm_binary(c, e);
    } else if (e->kind == CB_EXPR_LIT) {
        return cm_literal(c, &e->lit);
    } else {
        panic("identifier not implemented");
    }
}

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

#define kind_is_numeric(k) ((k) == CB_PRIM_INTEGER || (k) == CB_PRIM_REAL)

const char TYPE_TABLE[] = {
    [CB_PRIM_NULL] = 'w', [CB_PRIM_INTEGER] = 'l',
    [CB_PRIM_REAL] = 'd', [CB_PRIM_BOOLEAN] = 'w',
    [CB_PRIM_CHAR] = 'w', [CB_PRIM_STRING] = 'l', // TODO: string stuff
};

Val cm_binary(Compiler* c, CB_Expr* e) {
    Val lhs = cm_expr(c, e->lhs), rhs = cm_expr(c, e->rhs);
    Pos lhs_pos = e->lhs->pos, rhs_pos = e->rhs->pos;

    if (!lhs.have)
        goto fail;

    if (!rhs.have)
        goto fail;

    if (lhs.kind > CB_PRIM_CUSTOM || rhs.kind > CB_PRIM_CUSTOM) {
        cm_diag(c, e->pos, "custom types not implemented for binary exprs!");
        goto fail;
    }

    switch (e->kind) {
        case CB_EXPR_ADD: {
            if (lhs.kind == CB_PRIM_STRING || rhs.kind == CB_PRIM_STRING) {
                panic("string concatenation not implemented");
                goto start_compiling; // we don't wanna trigger the bottom
                                      // numeric check
            }
        } // fallthrough
        case CB_EXPR_SUB:
        case CB_EXPR_MUL:
        case CB_EXPR_DIV:
        case CB_EXPR_POW:
        case CB_EXPR_LT:
        case CB_EXPR_GT:
        case CB_EXPR_LEQ:
        case CB_EXPR_GEQ: {
            if (!kind_is_numeric(lhs.kind) || !kind_is_numeric(rhs.kind))
                goto type_error;
        } break;
        case CB_EXPR_SHL:
        case CB_EXPR_SHR: {
            if (lhs.kind != CB_PRIM_INTEGER || rhs.kind != CB_PRIM_INTEGER)
                goto type_error;
        } break;
        case CB_EXPR_AND:
        case CB_EXPR_OR: {
            if (lhs.kind != CB_PRIM_BOOLEAN || rhs.kind != CB_PRIM_BOOLEAN)
                goto type_error;
        } break;
        case CB_EXPR_BITOR:
        case CB_EXPR_BITAND:
        case CB_EXPR_BITXOR: {
            panic("not implemented");
        } break;
        default: {
            panic("not implemented");
        } break;
    }

start_compiling:
    usize id = c->id++;

    // gotta figure the types out
    CB_Type res_kind = lhs.kind;
    char ct = TYPE_TABLE[res_kind];

    // do integer-real promotion
    // if it happens, we steal the current id and make a new one
    if (lhs.kind == CB_PRIM_INTEGER && rhs.kind == CB_PRIM_REAL) {
        lhs.kind = CB_PRIM_REAL;

        cm_writefln(c, "%%r%zu =d sltof %%r%zu", id, lhs.id);
        lhs.id = id++; // we need a new one afterwards

        res_kind = CB_PRIM_REAL;
        ct = TYPE_TABLE[CB_PRIM_REAL];
    } else if (lhs.kind == CB_PRIM_REAL && rhs.kind == CB_PRIM_INTEGER) {
        rhs.kind = CB_PRIM_REAL;

        cm_writefln(c, "%%r%zu =d sltof %%r%zu", id, rhs.id);
        rhs.id = id++; // we need a new one now

        res_kind = CB_PRIM_REAL;
        ct = TYPE_TABLE[CB_PRIM_REAL];
    }

    switch (e->kind) {
        case CB_EXPR_ADD: {
            if (lhs.kind == CB_PRIM_STRING || rhs.kind == CB_PRIM_STRING) {
                panic("string concatenation not implemented");
            }

            cm_writefln(c, "%%r%zu =%c add %%r%zu, %%r%zu", id, ct, lhs.id,
                        rhs.id);
        } break;
        case CB_EXPR_SUB: {
            cm_writefln(c, "%%r%zu =%c sub %%r%zu, %%r%zu", id, ct, lhs.id,
                        rhs.id);
        } break;
        case CB_EXPR_MUL: {
            cm_writefln(c, "%%r%zu =%c mul %%r%zu, %%r%zu", id, ct, lhs.id,
                        rhs.id);
        } break;
        case CB_EXPR_DIV: {
            if (lhs.kind == CB_PRIM_INTEGER) {
                lhs.kind = CB_PRIM_REAL;
                cm_writefln(c, "%%r%zu =d sltof %%r%zu", id, lhs.id);
                lhs.id = id++;
            }
            if (rhs.kind == CB_PRIM_INTEGER) {
                rhs.kind = CB_PRIM_REAL;
                cm_writefln(c, "%%r%zu =d sltof %%r%zu", id, rhs.id);
                rhs.id = id++;
            }
            res_kind = CB_PRIM_REAL;
            cm_writefln(c, "%%r%zu =d div %%r%zu, %%r%zu", id, lhs.id, rhs.id);
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
        default: panic("tried to compile non binary expr as binary");
    }

    return val(id, res_kind);
type_error:
    cm_diag(c, e->pos,
            "cannot perform binary operation %s with types %s and %s!",
            expr_kind_string(e->kind), type_string(lhs.kind),
            type_string(rhs.kind));
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

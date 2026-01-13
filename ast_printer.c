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
#include <stdio.h>
#include <stdlib.h> // used in macro

#include "a_string.h"
#include "ast.h"
#include "ast_printer.h"
#include "common.h"

static const char* EXPR_KIND_STRINGS[] = {
    [CB_EXPR_LIT] = "lit",
    [CB_EXPR_IDENT] = "ident",
    [CB_EXPR_NEGATION] = "neg",
    [CB_EXPR_NOT] = "not",
    [CB_EXPR_GROUPING] = "grouping",
    [CB_EXPR_TYPECAST] = "typecast",
    [CB_EXPR_FNCALL] = "fncall",
    [CB_EXPR_ADD] = "add",
    [CB_EXPR_SUB] = "sub",
    [CB_EXPR_MUL] = "mul",
    [CB_EXPR_DIV] = "div",
    [CB_EXPR_POW] = "pow",
    [CB_EXPR_DEREF] = "deref",
    [CB_EXPR_REF] = "ref",
    [CB_EXPR_BITNOT] = "bitnot",
    [CB_EXPR_LT] = "lt",
    [CB_EXPR_GT] = "gt",
    [CB_EXPR_LEQ] = "leq",
    [CB_EXPR_GEQ] = "geq",
    [CB_EXPR_EQ] = "eq",
    [CB_EXPR_NEQ] = "neq",
    [CB_EXPR_SHL] = "shl",
    [CB_EXPR_SHR] = "shr",
    [CB_EXPR_OR] = "or",
    [CB_EXPR_AND] = "and",
    [CB_EXPR_BITOR] = "bitor",
    [CB_EXPR_BITAND] = "bitand",
    [CB_EXPR_BITXOR] = "bitxor",
};

static const char* expr_kind_string(CB_ExprKind k) {
    // TODO: do a range check instead
    const char* res = EXPR_KIND_STRINGS[k];
    if (res)
        return res;
    else
        panic("invalid expr kind %d passed to string", (i32)k);
}

void ap_write_stdout(AstPrinter* p, const char* data) {
    printf("%s", data);
}

void ap_write_stderr(AstPrinter* p, const char* data) {
    eprintf("%s", data);
}

void ap_write_file(AstPrinter* p, const char* data) {
    fprintf(p->fp, "%s", data);
}

void ap_write_string(AstPrinter* p, const char* data) {
    as_append_cstr(&p->buf, data);
}

#define AP_WRITEF_MAXSIZE 512
static void ap_writefer(AstPrinter* p, const char* data, ...) {
    char buf[AP_WRITEF_MAXSIZE] = {0};
    va_list lst;
    va_start(lst, data);
    vsprintf(buf, data, lst);
    va_end(lst);
    p->write(p, buf);
}

AstPrinter ap_new(void) {
    return ap_new_with_stdout_writer();
}

AstPrinter ap_new_with_stdout_writer(void) {
    return (AstPrinter){
        .write = ap_write_stdout,
        .writef = ap_writefer,
    };
}

AstPrinter ap_new_with_stderr_writer(void) {
    return (AstPrinter){
        .write = ap_write_stderr,
        .writef = ap_writefer,
    };
}

AstPrinter ap_new_with_file_writer(FILE* fp) {
    return (AstPrinter){
        .write = ap_write_file,
        .writef = ap_writefer,
        .fp = fp,
    };
}

AstPrinter ap_new_with_string_writer(void) {
    return (AstPrinter){
        .write = ap_write_string,
        .writef = ap_writefer,
        .buf = as_with_capacity(128),
    };
}

void ap_visit_value(AstPrinter* p, const CB_Value* val) {
    if (val->kind > CB_PRIM_CUSTOM) {
        p->writef(p, "{TYPE %d}", (i32)val->kind);
    }

    switch (val->kind) {
        case CB_PRIM_NULL: {
            p->write(p, "NULL");
        } break;
        case CB_PRIM_INTEGER: {
            p->writef(p, "%d", val->integer);
        } break;
        case CB_PRIM_REAL: {
            p->writef(p, "%lf", val->real);
        } break;
        case CB_PRIM_BOOLEAN: {
            if (val->boolean) {
                p->write(p, "true");
            } else {
                p->write(p, "false");
            }
        } break;
        case CB_PRIM_CHAR: {
            p->writef(p, "%c", val->chr);
        } break;
        case CB_PRIM_STRING: {
            p->writef(p, "%.*s", as_fmt(val->string));
        } break;
    }
}

void ap_visit_expr(AstPrinter* p, CB_Expr* e) {
    const char* kind = expr_kind_string(e->kind);
    p->writef(p, "%s(", kind);
    if (cb_expr_kind_is_binary(e->kind)) {
        ap_visit_expr(p, e->lhs);
        p->write(p, ", ");
        ap_visit_expr(p, e->rhs);
    } else {
        if (cb_expr_kind_is_unary(e->kind)) {
            ap_visit_expr(p, e->unary);
        } else if (e->kind == CB_EXPR_LIT) {
            ap_visit_value(p, &e->lit);
        } else {
            p->writef(p, "%.*s", as_fmt(e->ident));
        }
    }
    p->write(p, ")");
}

void ap_visit_stmt(AstPrinter* p, CB_Stmt* s);
void ap_visit_output_stmt(AstPrinter* p, CB_OutputStmt* s);
void ap_visit_input_stmt(AstPrinter* p, CB_InputStmt* s);

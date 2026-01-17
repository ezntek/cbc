/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <string.h>
#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> // used in macro

#include "a_string.h"
#include "a_vector.h"
#include "ast.h"
#include "common.h"
#include "compiler.h"

Compiler cm_new(void) {
    Compiler res = {
        .writer_state.mode = CM_WRITER_MODE_STDOUT,
    };
    return res;
}

bool cm_new_with_file_writer(const char* filename, Compiler* out) {
    FILE* ptr = fopen(filename, "w");
    if (!ptr) {
        perror("fopen");
        return false;
    }

    Compiler res = {0};
    res.writer_state.fp = ptr;
    res.writer_state.mode = CM_WRITER_MODE_FILE;

    *out = res;
    return true;
}

Compiler cm_new_with_string_writer() {
    Compiler res = {0};
    res.writer_state.buf = as_with_capacity(128);
    res.writer_state.mode = CM_WRITER_MODE_STRING;
    return res;
}

void cm_free(Compiler* c) {
    switch (c->writer_state.mode) {
        case CM_WRITER_MODE_FILE: {
            fclose(c->writer_state.fp);
        } break;
        case CM_WRITER_MODE_STRING: {
            as_free(&c->writer_state.buf);
        } break;
        default: break;
    }

    for (usize i = 0; i < c->ss.len; i++) {
        as_free(&c->ss.data[i]);
    }

    av_free(&c->ss);
}

Val val(const char* data) {
    Val v = {0};
    memcpy(v.data, data, 8);
    return v;
}

// writer functions

static void cm_write(Compiler* c, const char* s) {
    switch (c->writer_state.mode) {
        case CM_WRITER_MODE_STDOUT: {
            printf("%s", s);
        } break;
        case CM_WRITER_MODE_FILE: {
            fprintf(c->writer_state.fp, "%s", s);
        } break;
        case CM_WRITER_MODE_STRING: {
            as_append_cstr(&c->writer_state.buf, s);
        } break;
    }
}

#define CM_WRITEF_MAXSIZE 1024
static char writef_buf[CM_WRITEF_MAXSIZE] = {0};

static void cm_writef(Compiler* c, const char* restrict format, ...) {
    va_list lst;
    va_start(lst, format);

    switch (c->writer_state.mode) {
        case CM_WRITER_MODE_STDOUT: {
            vfprintf(stdout, format, lst);
        } break;
        case CM_WRITER_MODE_FILE: {
            vfprintf(c->writer_state.fp, format, lst);
        } break;
        case CM_WRITER_MODE_STRING: {
            vsnprintf(writef_buf, CM_WRITEF_MAXSIZE - 1, format, lst);
            as_append_cstr(&c->writer_state.buf, writef_buf);
        } break;
    }

    va_end(lst);
}

static char name_buf[8] = {0};

static Val cm_literal(Compiler* c, CB_Value* e) {
    switch (e->kind) {
        case CB_PRIM_STRING: {
            usize idx = c->ss.len;
            av_append(&c->ss, as_slice_cstr(e->string.data, 0, e->string.len));
            snprintf(name_buf, 8, "$s%zX", idx);
            Val res = val(name_buf);
            return res;
        } break;
        default: {
            panic("not implemented");
        } break;

            /*
            case CB_PRIM_INTEGER: {

            } break;
            case CB_PRIM_REAL: {

            } break;
            case CB_PRIM_BOOLEAN: {

            } break;
            case CB_PRIM_CHAR: {

            } break;
            case CB_PRIM_NULL: {

            } break;
            */
    }
}

Val cm_expr(Compiler* c, CB_Expr* e) {
    if (cb_expr_kind_is_unary(e->kind)) {
        panic("unary not implemented");
    } else if (cb_expr_kind_is_binary(e->kind)) {
        panic("binary not implemented");
    } else if (e->kind == CB_EXPR_LIT) {
        return cm_literal(c, &e->lit);
    } else {
        panic("identifier not implemented");
    }
}

static void cm_output_stmt(Compiler* c, CB_Stmt* s) {
    for (usize i = 0; i < s->output.len; i++) {
        Val v = cm_expr(c, &s->output.exprs[i]);
        cm_writef(c, "%%r =w call $puts(l %s)\n", v.data);
    }
}

void cm_stmt(Compiler* c, CB_Stmt* s) {
    switch (s->kind) {
        case CB_STMT_OUTPUT: {
            cm_output_stmt(c, s);
        } break;
        default: panic("statement %d not implemented", s->kind);
    }
}

void cm_program(Compiler* c, CB_Program* prog) {
    cm_write(c, "export function w $main() {\n@start\n");
    for (usize i = 0; i < prog->len; i++) {
        cm_stmt(c, &prog->stmts[i]);
    }
    cm_write(c, "ret 0\n}\n");
    for (usize i = 0; i < c->ss.len; i++) {
        cm_writef(c, "data $s%zu = align 1 { b \"", i);
        // TODO: escape sequences
        cm_writef(c, "%.*s", as_fmt(c->ss.data[i]));
        cm_write(c, "\", b 0 }\n");
    }
}

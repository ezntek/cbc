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
#include "../ast.h"
#include "../common.h"
#include "compiler.h"
#include "compiler_internal.h"

static void cm_output_stmt(Compiler* c, CB_Stmt* s) {
    for (usize i = 0; i < s->output.len; i++) {
        CB_Expr* e = &s->output.exprs[i];
        Val v = cm_expr(c, e);
        switch (v.kind) {
            case CB_PRIM_STRING: {
                cm_writefln(c, "call $printf(l $__FS, ..., l %%r%zu)", v.id);
            } break;
            case CB_PRIM_INTEGER: {
                cm_writefln(c, "call $printf(l $__FI, ..., l %%r%zu)", v.id);
            } break;
            case CB_PRIM_REAL: {
                // TODO: fix precision bug
                cm_writefln(c, "call $printf(l $__FR, ..., d %%r%zu)", v.id);
            } break;
            case CB_PRIM_BOOLEAN: {
                cm_writefln(c, "call $__PRINT_BOOLEAN(w %%r%zu)", v.id);
            } break;
            case CB_PRIM_CHAR: {
                cm_writefln(c, "call $printf(l $__FC, ..., w %%r%zu)", v.id);
            } break;
            default: {
                panic("outputting type \"%s\" not implemented",
                      type_string(v.kind));
            } break;
                /*
                case CB_PRIM_NULL: {

                } break;
                */
        }
    }

    cm_writefln(c, "call $putchar(w 10)\n");
}

void cm_stmt(Compiler* c, CB_Stmt* s) {
    switch (s->kind) {
        case CB_STMT_OUTPUT: {
            cm_output_stmt(c, s);
        } break;
        default: panic("statement %d not implemented", s->kind);
    }
}

static void write_utils(Compiler* c) {
    cm_write(c, "function $__PRINT_BOOLEAN(w %val) {\n"
                "@start\n"
                "%fp =l loadl $stdout\n"
                "jnz %val, @t, @f\n"
                "@t\n"
                "call $fputs(l $__FTRUE, l %fp)\n"
                "jmp @e\n"
                "@f\n"
                "call $fputs(l $__FFALSE, l %fp)\n"
                "@e\n"
                "ret\n"
                "}\n");
}

static void write_format_specifiers(Compiler* c) {
    cm_writeln(c, "data $__FS = { b \"%s\", b 0 }\n"
                  "data $__FI = { b \"%li\", b 0 }\n"
                  "data $__FR = { b \"%g\", b 0 }\n"
                  "data $__FC = { b \"%c\", b 0 }\n"
                  "data $__FFALSE = { b \"FALSE\", b 0 }\n"
                  "data $__FTRUE = { b \"TRUE\", b 0 }\n");
}

#define MAX_ERROR_COUNT 20
bool cm_program(Compiler* c, CB_Program* prog, a_string* file_name) {
    if (file_name)
        c->file_name = *file_name;

    write_utils(c);
    cm_writeln(c, "export function w $main() {\n@start");
    for (usize i = 0; i < prog->len; i++) {
        cm_stmt(c, &prog->stmts[i]);

        if (c->error_count > MAX_ERROR_COUNT) {
            cm_diag(c, prog->stmts[i].pos,
                    "too many errors reported, stopping now.");
            return false;
        }
    }

    if (c->error_count) {
        cm_diag(c, BEGIN_POS, "errors were reported.");
        return false;
    }

    cm_writeln(c, "ret 0\n}");

    for (usize i = 0; i < c->ss.len; i++) {
        cm_writef(c, "data $__S%zu = align 1 { b \"", i);
        // TODO: escape sequences
        cm_writef(c, "%.*s", as_fmt(c->ss.data[i]));
        cm_writeln(c, "\", b 0 }\n");
    }

    write_format_specifiers(c);

    return true;
}

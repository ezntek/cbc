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
#include <string.h>

#include "../3rdparty/uthash.h"
#include "../a_string.h"
#include "../a_vector.h"
#include "../ast.h"
#include "../common.h"
#include "compiler.h"
#include "compiler_internal.h"

Compiler cm_new(void) {
    return (Compiler){.out = as_with_capacity(256)};
}

void cm_free(Compiler* c) {
    as_free(&c->out);

    for (usize i = 0; i < c->ss.len; i++) {
        as_free(&c->ss.data[i]);
    }

    av_free(&c->ss);
}

void cm_diag(Compiler* c, Pos pos, const char* restrict format, ...) {
    if (c->file_name.data) {
        eprintf("\033[31;1merror: \033[0;1m%.*s:%u:%u: \033[0m",
                (int)c->file_name.len, c->file_name.data, pos.row, pos.col);
    } else {
        eprintf("\033[31;1merror: \033[0;1m%u:%u: \033[0m", pos.row, pos.col);
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);

    c->error_count++;
}

// var hash table functions

void cm_var_table_add(Compiler* c, const char* name, usize len, CB_Type typ,
                      bool is_const) {
    VarDecl* entry;

    HASH_FIND(hh, c->vt, name, len, entry);

    if (!entry) {
        entry = calloc(1, sizeof(VarDecl));
        check_alloc(entry);
        entry->name = calloc(len, 1);
        check_alloc(entry->name);
        memcpy(entry->name, name, len);
        entry->len = len;

        HASH_ADD_KEYPTR(hh, c->vt, entry->name, len, entry);
    }

    entry->typ = typ;
    entry->is_const = is_const;
}

VarDecl* cm_var_table_find(Compiler* c, const char* name, usize len) {
    VarDecl* entry;
    HASH_FIND(hh, c->vt, name, len, entry);
    return entry;
}

void cm_var_table_delete(Compiler* c, const char* name, usize len) {
    VarDecl* entry = cm_var_table_find(c, name, len);
    if (!entry)
        return;

    HASH_DEL(c->vt, entry);
    free(entry->name);
    free(entry);
}

void cm_var_table_free(Compiler* c) {
    VarDecl *entry, *tmp;

    HASH_ITER(hh, c->vt, entry, tmp) {
        HASH_DEL(c->vt, entry);
        free(entry->name);
        free(entry);
    }
}

// writer functions

void cm_write(Compiler* c, const char* s) {
    as_append_cstr(&c->out, s);
}

void cm_writeln(Compiler* c, const char* s) {
    as_append_cstr(&c->out, s);
    as_append_char(&c->out, '\n');
}

#define CM_WRITEF_MAXSIZE 1024
static char writef_buf[CM_WRITEF_MAXSIZE] = {0};

void cm_writef(Compiler* c, const char* restrict format, ...) {
    va_list lst;
    va_start(lst, format);

    vsnprintf(writef_buf, CM_WRITEF_MAXSIZE - 1, format, lst);
    as_append_cstr(&c->out, writef_buf);

    va_end(lst);
}

void cm_writefln(Compiler* c, const char* restrict format, ...) {
    va_list lst;
    va_start(lst, format);

    vsnprintf(writef_buf, CM_WRITEF_MAXSIZE - 1, format, lst);
    as_append_cstr(&c->out, writef_buf);
    as_append_char(&c->out, '\n');

    va_end(lst);
}

#define TYPE_STRING_BUFSZ 64
static char type_string_buf[TYPE_STRING_BUFSZ] = {0};
static const char* PRIM_TYPE_TABLE[] = {
    [CB_PRIM_NULL] = "NULL", [CB_PRIM_INTEGER] = "INTEGER",
    [CB_PRIM_REAL] = "REAL", [CB_PRIM_BOOLEAN] = "BOOLEAN",
    [CB_PRIM_CHAR] = "CHAR", [CB_PRIM_STRING] = "STRING",
};

const char* type_string(CB_Type t) {
    if (t <= CB_PRIM_STRING)
        strcpy(type_string_buf, PRIM_TYPE_TABLE[t]);
    else
        snprintf(type_string_buf, TYPE_STRING_BUFSZ, "Type %u", (u32)t);

    return type_string_buf;
}

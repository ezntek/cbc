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

#include "../a_string.h"
#include "../a_vector.h"
#include "../ast.h"
#include "../common.h"
#include "compiler.h"
#include "compiler_internal.h"

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

// writer functions

void cm_write(Compiler* c, const char* s) {
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

void cm_writeln(Compiler* c, const char* s) {
    switch (c->writer_state.mode) {
        case CM_WRITER_MODE_STDOUT: {
            printf("%s\n", s);
        } break;
        case CM_WRITER_MODE_FILE: {
            fprintf(c->writer_state.fp, "%s\n", s);
        } break;
        case CM_WRITER_MODE_STRING: {
            as_append_cstr(&c->writer_state.buf, s);
            as_append_char(&c->writer_state.buf, '\n');
        } break;
    }
}

#define CM_WRITEF_MAXSIZE 1024
static char writef_buf[CM_WRITEF_MAXSIZE] = {0};

void cm_writef(Compiler* c, const char* restrict format, ...) {
    va_list lst;
    va_start(lst, format);

    switch (c->writer_state.mode) {
        case CM_WRITER_MODE_STDOUT: {
            vprintf(format, lst);
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

void cm_writefln(Compiler* c, const char* restrict format, ...) {
    va_list lst;
    va_start(lst, format);

    switch (c->writer_state.mode) {
        case CM_WRITER_MODE_STDOUT: {
            vprintf(format, lst);
            putchar('\n');
        } break;
        case CM_WRITER_MODE_FILE: {
            vfprintf(c->writer_state.fp, format, lst);
            fprintf(c->writer_state.fp, "\n");
        } break;
        case CM_WRITER_MODE_STRING: {
            vsnprintf(writef_buf, CM_WRITEF_MAXSIZE - 1, format, lst);
            as_append_cstr(&c->writer_state.buf, writef_buf);
            as_append_char(&c->writer_state.buf, '\n');
        } break;
    }

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
    const char* s = PRIM_TYPE_TABLE[t];

    if (s)
        strcpy(type_string_buf, s);
    else
        snprintf(type_string_buf, TYPE_STRING_BUFSZ, "Type %u", (u32)t);

    return type_string_buf;
}

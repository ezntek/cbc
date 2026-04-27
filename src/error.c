/*
 * beancode: a portable IGCSE Computer Science (0478, 0984, 2210) Pseudocode
 * interpreter.
 *
 * Copyright (c) Eason Qin, 2025-2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "a_string.h"
#include "a_string_slice.h"
#include "common.h"
#include "error.h"

static const char* ERROR_KIND_STRINGS[] = {
    [CBC_ERROR_EOF] = "EOFError",
    [CBC_ERROR_RUNTIME] = "RuntimeError",
    [CBC_ERROR_SYNTAX] = "SyntaxError",
};

static char error_kind_buf[32] = {0};

a_string_slice cbc_error_kind_to_string_slice(CBCErrorKind k) {
    strcpy(error_kind_buf, ERROR_KIND_STRINGS[k]);
    return ass_from_cstr(error_kind_buf);
}

a_string cbc_error_kind_to_string(CBCErrorKind k) {
    return astr(ERROR_KIND_STRINGS[k]);
}

CBCError cbc_error_new(CBCErrorKind k, CBCPos p, a_string msg) {
    return (CBCError){
        .kind = k,
        .pos = p,
        .msg = msg,
    };
}

CBCError cbc_error_new_cstr(CBCErrorKind k, CBCPos p, const char* msg) {
    return (CBCError){
        .kind = k,
        .pos = p,
        .msg = astr(msg),
    };
}

CBCError cbc_error_new_string_slice(CBCErrorKind k, CBCPos p, a_string_slice msg) {
    return (CBCError){
        .kind = k,
        .pos = p,
        .msg = as_from_string_slice(msg),
    };
}

void cbc_error_free(CBCError* err) {
    as_free(&err->msg);
}

void __cbc_error_print_impl(CBCError* err, struct __cbc_error_print_opts opts) {
    if (!opts.f) opts.f = stderr;

    if (opts.no_color) {
        fprintf(opts.f, "%.*s:%u:%u: error: ", as_fmt(opts.file_name),
                err->pos.row, err->pos.col);
    } else {
        fprintf(opts.f, "\033[1m%.*s:%u:%u \033[31;1merror: \033[0m",
                as_fmt(opts.file_name), err->pos.row, err->pos.col);
    }

    usize len = 0;

    for (; len < err->msg.len && err->msg.data[len] != '\n'; len++)
        continue;

    fprintf(opts.f, "%.*s\n", (int)len, err->msg.data);

    // TODO: print context

    if (ass_valid(opts.src)) {
        panic("shawarma %d", 69);
        // TODO: source code printer
    }
}

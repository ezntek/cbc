/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef CBC_ERROR_H
#define CBC_ERROR_H

#include <stdbool.h>

#include "a_string.h"
#include "a_string_slice.h"
#include "lexer_types.h"

typedef enum {
    CBC_ERROR_BOGUS = 0,
    CBC_ERROR_EOF,
    CBC_ERROR_SYNTAX,
    CBC_ERROR_RUNTIME,
} CBCErrorKind;

a_string_slice cbc_error_kind_to_string_slice(CBCErrorKind k);
a_string cbc_error_kind_to_string(CBCErrorKind k);

typedef struct {
    CBCErrorKind kind;
    CBCPos pos;
    a_string msg; // rows delimited by 0xA
} CBCError;

CBCError cbc_error_new(CBCErrorKind k, CBCPos p);
CBCError cbc_error_new_astr(CBCErrorKind k, CBCPos p, a_string msg);
CBCError cbc_error_new_cstr(CBCErrorKind k, CBCPos p, const char* msg);
CBCError cbc_error_new_string_slice(CBCErrorKind k, CBCPos p,
                                    a_string_slice msg);

struct __cbc_error_print_opts {
    a_string_slice file_name;
    a_string_slice src;
    FILE* f;
    bool no_color;
};

// void cbc_error_print(CBCError* err, a_string_slice file_name, ...)
#define cbc_error_print(err, fname, ...)                                       \
    __cbc_error_print_impl((err), (struct __cbc_error_print_opts){             \
                                      .file_name = fname, __VA_ARGS__})

void __cbc_error_print_impl(CBCError* err, struct __cbc_error_print_opts opts);

void cbc_error_free(CBCError* err);

#endif // CBC_ERROR_H

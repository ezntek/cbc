/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _COMPILER_H
#define _COMPILER_H

#include "a_vector.h"
#include "ast.h"
#include "common.h"

typedef enum {
    CM_WRITER_MODE_STDOUT = 0,
    CM_WRITER_MODE_FILE,
    CM_WRITER_MODE_STRING,
} CompilerWriterMode;

// array of owned slices
AV_DECL(a_string, StringStorage)

typedef struct {
    CompilerWriterMode mode;
    union {
        FILE* fp; // NULL if file writer not used
        a_string buf;
    };
} CompilerWriterState;

typedef struct {
    CompilerWriterState writer_state;
    StringStorage ss;
    usize string_id;
    usize id;
} Compiler;

typedef union {
    char data[8];
    u64 v;
} Val;

Val val(const char* data);

Compiler cm_new(void);
void cm_free(Compiler* c);
bool cm_new_with_file_writer(const char* filename, Compiler* out);
Compiler cm_new_with_string_writer();

Val cm_expr(Compiler* c, CB_Expr* e);
void cm_stmt(Compiler* c, CB_Stmt* s);
void cm_program(Compiler* c, CB_Program* prog);

#endif // _COMPILER_H

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

#include "../a_vector.h"
#include "../ast.h"
#include "../common.h"

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
    a_string file_name;
    usize string_id;
    usize label_id;
    usize id;
    u32 error_count;
} Compiler;

typedef struct {
    u64 id;
    CB_Type kind; // 4 bytes
    bool have;    // if set to false, it is invalid and there was an error.
} Val;

Compiler cm_new(void);
void cm_free(Compiler* c);

void cm_diag(Compiler* c, Pos pos, const char* restrict format, ...);

bool cm_new_with_file_writer(const char* filename, Compiler* out);
Compiler cm_new_with_string_writer();

Val cm_expr(Compiler* c, CB_Expr* e);
void cm_stmt(Compiler* c, CB_Stmt* s);
// NULL file name: not specified
bool cm_program(Compiler* c, CB_Program* prog, a_string* file_name);

#endif // _COMPILER_H

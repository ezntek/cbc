/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _COMPILER_INTERNAL_H
#define _COMPILER_INTERNAL_H

#include "compiler.h"

void cm_diag(Compiler* c, Pos pos, const char* restrict format, ...);
void cm_write(Compiler* c, const char* s);
void cm_writeln(Compiler* c, const char* s);
void cm_writef(Compiler* c, const char* restrict format, ...);
void cm_writefln(Compiler* c, const char* restrict format, ...);
const char* type_string(CB_Type t);

#define val(id, t)                                                             \
    (Val) {                                                                    \
        .have = 1, .id = (id), .kind = (t)                                     \
    }

#define BEGIN_POS (Pos){.col = 1, .row = 1}

#endif

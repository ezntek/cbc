/*
 * cbc: a cursed bean(code) compiler
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2026.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _AST_PRINTER_H
#define _AST_PRINTER_H

#include "ast.h"

typedef struct AstPrinter AstPrinter;

// boilerplate

typedef void (*AstPrinterWriter)(AstPrinter*, const char*);
typedef void (*AstPrinterWritefer)(AstPrinter*, const char*, ...);

typedef struct AstPrinter {
    AstPrinterWriter write;
    AstPrinterWritefer writef;
    FILE* fp;     // null if file writer is not used
    a_string buf; // for the string writer
    u32 indent;
} AstPrinter;

void ap_write_stdout(AstPrinter* p, const char* data);
void ap_write_stderr(AstPrinter* p, const char* data);
void ap_write_file(AstPrinter* p, const char* data);
void ap_write_string(AstPrinter* p, const char* data);

AstPrinter ap_new(void);
AstPrinter ap_new_with_stdout_writer(void);
AstPrinter ap_new_with_stderr_writer(void);
AstPrinter ap_new_with_file_writer(FILE* fp);
AstPrinter ap_new_with_string_writer(void);

void ap_visit_value(AstPrinter* p, const CB_Value* val);

void ap_visit_expr(AstPrinter* p, CB_Expr* e);

void ap_visit_stmt(AstPrinter* p, CB_Stmt* s);
void ap_visit_output_stmt(AstPrinter* p, CB_OutputStmt* s);
void ap_visit_input_stmt(AstPrinter* p, CB_InputStmt* s);

// actual visit functions

#endif // _AST_PRINTER_H

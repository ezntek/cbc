/*
 * a_string/a_vector: a scuffed dynamic vector/string implementation.
 *
 * Copyright (c) Eason Qin, 2025.
 *
 * This source code form is licensed under the MIT/Expat license.
 * Visit the OSI website for a digital version.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "a_string.h"
#include "common.h"

a_string as_new(void) {
    return as_with_capacity(8);
}

a_string as_with_capacity(usize cap) {
    a_string res = {.len = 0, .cap = cap};

    res.data = calloc(res.cap, 1); // sizeof(char)
    check_alloc(res.data);

    return res;
}

void as_clear(a_string* s) {
    memset(s->data, '\0', s->cap);
}

void as_free(a_string* s) {
    if (!as_valid(s)) {
        return;
    }

    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

void as_copy(a_string* dest, const a_string* src) {
    if (!as_valid(dest))
        panic("cannot operate on invalid a_string!");
    if (!as_valid(src))
        panic("source string is invalid!");

    if (src->len > dest->cap) {
        as_reserve(dest, src->cap);
    }

    strcpy(dest->data, src->data);
    dest->len = src->len;
}

void as_copy_cstr(a_string* dest, const char* src) {
    if (!as_valid(dest))
        panic("cannot operate on invalid a_string!");
    if (src == NULL)
        panic("source C string is null!");

    usize len = strlen(src);
    if (len + 1 > dest->cap) {
        as_reserve(dest, len);
    }

    strncpy(dest->data, src, len);
    dest->len = len;
    if (dest->data[dest->len] != '\0') {
        dest->data[dest->len] = '\0'; // always nullterm
    }
}

void as_ncopy(a_string* dest, const a_string* src, usize chars) {
    if (!as_valid(dest))
        panic("cannot operate on invalid a_string!");
    if (!as_valid(src))
        panic("source string is invalid!");

    if (chars > dest->cap) {
        as_reserve(dest, chars);
    }
    as_clear(dest);

    stpncpy(dest->data, src->data, chars);
    dest->len = chars;
}

void as_ncopy_cstr(a_string* dest, const char* src, usize chars) {
    if (!as_valid(dest))
        panic("cannot operate on invalid a_string!");
    if (src == NULL)
        panic("source C string is null!");

    if (chars + 1 > dest->cap) {
        as_reserve(dest, chars + 1);
    }
    as_clear(dest);

    stpncpy(dest->data, src, chars);
    dest->len = chars;
}

void as_reserve(a_string* s, usize cap) {
    if (!as_valid(s)) {
        panic("the string is invalid");
    }

    if (s->cap == cap)
        return;

    s->data = realloc(s->data, cap);
    check_alloc(s->data);
    s->cap = cap;
}

a_string as_from_cstr(const char* cstr) {
    if (cstr == NULL)
        panic("source C string is null!");

    a_string res = {
        .data = NULL,
        .cap = strlen(cstr) + 1,
        .len = strlen(cstr),
    };

    res.data = calloc(res.cap, 1); // sizeof(char)
    strcpy(res.data, cstr);

    return res;
}

a_string astr(const char* cstr) {
    return as_from_cstr(cstr);
}

a_string as_dupe(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on invalid a_string!");

    a_string res = as_with_capacity(s->cap);
    res.len = s->len;
    strncpy(res.data, s->data, s->len);
    return res;
}

a_string as_asprintf(const char* restrict format, ...) {
    va_list args;
    va_start(args, format);

    va_list argscopy;
    va_copy(argscopy, args);

    usize len = vsnprintf(NULL, 0, format, argscopy);
    a_string res = as_with_capacity(len + 1);
    vsnprintf(res.data, res.cap, format, args);

    va_end(args);

    res.len = len;
    return res;
}

usize as_sprintf(a_string* dest, const char* restrict format, ...) {
    va_list args;
    va_start(args, format);

    va_list argscopy;
    va_copy(argscopy, args);
    usize len = vsnprintf(NULL, 0, format, argscopy);

    if (as_valid(dest)) {
        as_reserve(dest, len + 1);
    } else {
        *dest = as_with_capacity(len + 1);
    }
    as_clear(dest);

    usize res = vsnprintf(dest->data, dest->cap, format, args);

    va_end(args);

    dest->len = len;
    return res;
}

int as_fprint(const a_string* s, FILE* restrict stream) {
    return fprintf(stream, "%.*s", as_fmtp(s));
}

int as_fprintln(const a_string* s, FILE* restrict stream) {
    return fprintf(stream, "%.*s\n", as_fmtp(s));
}

int as_print(const a_string* s) {
    return as_fprint(s, stdout);
}

int as_println(const a_string* s) {
    return as_fprintln(s, stdout);
}

char* as_fgets(a_string* buf, usize cap, FILE* restrict stream) {
    usize actual_cap = (cap == 0) ? 8192 : cap;
    if (as_valid(buf)) {
        as_reserve(buf, actual_cap);
    } else {
        *buf = as_with_capacity(actual_cap);
    }
    char* fgets_res = fgets(buf->data, actual_cap, stream);
    if (fgets_res == NULL)
        return NULL;
    buf->len = strlen(buf->data);
    buf->data[buf->len] = 0;
    return buf->data;
}

bool as_read_line(a_string* buf, FILE* restrict stream) {
    char* res = as_fgets(buf, 0, stream);
    if (res == NULL)
        return false;

    // trim newline off
    if (as_last(buf) == '\n')
        as_pop(buf);

    // resize buf to appropriate size.
    as_reserve(buf, buf->len + 1);
    return true;
}

a_string as_read_file(const char* filename) {
    if (filename == NULL)
        panic("source file name C string is null!");

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        return (a_string){0};
    }

    fseek(fp, 0, SEEK_END);
    usize sz = ftell(fp);
    rewind(fp);
    a_string res = as_with_capacity(sz + 1);
    if (fread(res.data, 1, sz, fp) != sz) {
        return (a_string){0};
    }
    res.len = sz;
    res.data[res.len] = '\0';
    fclose(fp);
    return res;
}

a_string as_input(const char* prompt) {
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }

    a_string raw = as_new();
    if (!as_read_line(&raw, stdin))
        return (a_string){0};
    else
        return raw;
}

bool as_valid(const a_string* s) {
    return !(s->len == (usize)-1 || s->cap == (usize)-1 || s->data == NULL);
}

void as_append_char(a_string* s, char c) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    if (s->len + 1 > s->cap) {
        as_reserve(s, s->cap * 2);
    }

    s->data[s->len++] = c;
}

void as_append_cstr(a_string* s, const char* n) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    if (n == NULL)
        panic("null string passed to append operation!");

    usize new_len = strlen(n);
    usize required_cap = s->len + new_len + 1;
    if (required_cap > s->cap) {
        while (s->cap < required_cap) {
            as_reserve(s, s->cap * 2);
        }
    }

    for (usize i = 0; i < new_len; i++) {
        s->data[s->len++] = n[i];
    }
    s->data[s->len] = '\0'; // null terminate it
    s->len += new_len;
}

void as_append_astr(a_string* s, const a_string* n) {
    if (!as_valid(n))
        panic("a_string to be appended cannot be NULL!");

    as_append_cstr(s, n->data);
}

void as_append(a_string* s, const char* n) {
    as_append_cstr(s, n);
}

char as_pop(a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    char last = s->data[--s->len];
    s->data[s->len] = '\0';
    return last;
}

char as_at(const a_string* s, usize idx) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    if (idx >= s->len)
        panic("a_string index `%zu` out of range (length: `%zu`)!", idx,
              s->len);

    return s->data[idx];
}

char as_first(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    if (s->len == 0)
        panic("cannot get the first character of an empty a_string!");

    return s->data[0];
}

char as_last(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    if (s->len == 0)
        panic("cannot get the last character of an empty a_string!");

    return s->data[s->len - 1];
}

a_string as_trim_left(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    usize i = 0;
    while (i < s->len) { // must check this first, else segfault
        if (strchr(" \n\t\r", s->data[i])) {
            i++;
        } else {
            break;
        }
    }

    return as_from_cstr(&s->data[i]);
}

a_string as_trim_right(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    usize end = s->len - 1;
    while (strchr(" \n\t\r", s->data[end])) { // end >= 0 is always true
        end--;
    }
    end++;

    a_string res = as_with_capacity(end + 1);
    for (usize i = 0; i < end; i++) {
        res.data[i] = s->data[i];
    }

    return res;
}

a_string as_trim(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    usize begin = 0;
    usize end = s->len - 1;

    while (begin < s->len) { // must check this first, else segfault
        if (strchr(" \n\t\r", s->data[begin]) != NULL) {
            begin++;
        } else {
            break;
        }
    }

    while (strchr(" \n\t\r", s->data[end])) { // end >= 0 is always true
        end--;
    }

    end++;

    a_string res = as_with_capacity(end - begin + 1);
    res.len = end - begin;

    usize i = 0;
    for (usize j = begin; j < end; j++) {
        res.data[i++] = s->data[j];
    }

    return res;
}

void as_inplace_trim_left(a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    usize i = 0;
    while (i < s->len) { // must check this first, else segfault
        if (strchr(" \n\t\r", s->data[i])) {
            i++;
        } else {
            break;
        }
    }

    memmove(&s->data[0], &s->data[i], s->len - i);
}

void as_inplace_trim_right(a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    usize end = s->len - 1;
    while (strchr(" \n\t\r", s->data[end])) { // end >= 0 is always true
        end--;
    }
    end++;

    usize oldlen = s->len;
    s->len = end + 1;

    if (end > s->len)
        memset(&s->data[end + 1], 0, oldlen - end + 1);
}

void as_inplace_trim(a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    usize begin = 0;
    usize end = s->len - 1;

    while (begin < s->len) { // must check this first, else segfault
        if (isspace(s->data[begin])) {
            begin++;
        } else {
            break;
        }
    }

    while (isspace(s->data[end])) { // end >= 0 is always true
        end--;
    }

    end++;

    usize oldlen = s->len;
    s->len = end - begin;
    memmove(&s->data[0], &s->data[begin], s->len);
    if (end > s->len)
        memset(&s->data[end], 0, oldlen - s->len + 1);
}

a_string as_toupper(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    a_string res = as_with_capacity(s->len + 1);
    for (usize i = 0; i < s->len; i++) {
        res.data[i] = toupper(s->data[i]);
    }
    res.len = s->len;

    return res;
}

a_string as_tolower(const a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    a_string res = as_with_capacity(s->len + 1);
    for (usize i = 0; i < s->len; i++) {
        res.data[i] = tolower(s->data[i]);
    }
    res.len = s->len;

    return res;
}

void as_inplace_toupper(a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    for (usize i = 0; i < s->len; i++) {
        s->data[i] = toupper(s->data[i]);
    }
}

void as_inplace_tolower(a_string* s) {
    if (!as_valid(s))
        panic("cannot operate on an invalid a_string!");

    for (usize i = 0; i < s->len; i++) {
        s->data[i] = tolower(s->data[i]);
    }
}

bool as_equal(const a_string* lhs, const a_string* rhs) {
    if (!as_valid(lhs))
        panic("cannot compare an invalid a_string!");

    if (!as_valid(rhs))
        panic("cannot compare an invalid a_string!");

    if (lhs->len != rhs->len) {
        return false;
    }

    for (usize i = 0; i < lhs->len; i++) {
        if (lhs->data[i] != rhs->data[i]) {
            return false;
        }
    }

    return true;
}

bool as_equal_cstr(const a_string* lhs, const char* rhs) {
    if (!rhs)
        return false;
    a_string arhs = {
        .data = (char*)rhs,
        .len = strlen(rhs),
    }; // very sketchy, i know
    return as_equal(lhs, &arhs);
}

bool as_equal_case_insensitive(const a_string* lhs, const a_string* rhs) {
    if (!as_valid(lhs))
        panic("cannot compare an invalid a_string!");

    if (!as_valid(rhs))
        panic("cannot compare an invalid a_string!");

    if (lhs->len != rhs->len) {
        return false;
    }

    for (usize i = 0; i < lhs->len; i++) {
        if (tolower(lhs->data[i]) != tolower(rhs->data[i])) {
            return false;
        }
    }

    return true;
}

bool as_equal_case_insensitive_cstr(const a_string* lhs, const char* rhs) {
    if (!rhs)
        return false;
    a_string arhs = {
        .data = (char*)rhs,
        .len = strlen(rhs),
    }; // very sketchy, i know
    return as_equal_case_insensitive(lhs, &arhs);
}

a_string as_slice_cstr(const char* src, usize begin, usize end) {
    if (src == NULL)
        panic("source C string for slice operation is NULL!");

    if (begin > end)
        panic("begin cannot be greater than end in slice operation!");

    a_string res = as_new();
    as_ncopy_cstr(&res, &src[begin], end - begin);
    return res;
}

a_string as_slice(const a_string* src, usize begin, usize end) {
    if (!as_valid(src))
        panic("cannot slice an invalid a_string!");

    return as_slice_cstr(src->data, begin, end);
}

bool as_in(const a_string* needle, const a_string** haystack, usize len) {
    for (usize i = 0; i < len; ++i) {
        if (as_equal(needle, haystack[i]))
            return true;
    }
    return false;
}

bool as_in_cstr(const a_string* needle, const char** haystack, usize len) {
    for (usize i = 0; i < len; ++i) {
        if (as_equal_cstr(needle, haystack[i]))
            return true;
    }
    return false;
}

usize as_to_double(const a_string* src, double* res) {
    errno = 0;
    char* endptr = NULL;
    const char* nptr = src->data;
    double dbl = strtod(nptr, &endptr);
    usize diff = endptr - nptr;
    if (diff == src->len) {
        *res = dbl;
        return diff;
    } else {
        return diff;
    }
}

usize as_to_integer(const a_string* src, int64_t* res, int base) {
    errno = 0;
    char* endptr = NULL;
    const char* nptr = src->data;
    int64_t num = (int64_t)strtoll(nptr, &endptr, base);
    usize diff = endptr - nptr;
    if (diff == src->len) {
        *res = num;
        return diff;
    } else {
        return diff;
    }
}

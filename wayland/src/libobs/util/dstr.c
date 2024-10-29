/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <limits.h>

#include "c99defs.h"
#include "dstr.h"

void dstr_printf(struct dstr *dst, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    dstr_vprintf(dst, format, args);
    va_end(args);
}

void dstr_vprintf(struct dstr *dst, const char *format, va_list args)
{
    va_list args_cp;
    va_copy(args_cp, args);

    int len = vsnprintf(NULL, 0, format, args_cp);
    va_end(args_cp);

    if (len < 0)
        len = 4095;

    dstr_ensure_capacity(dst, ((size_t)len) + 1);
    len = vsnprintf(dst->array, ((size_t)len) + 1, format, args);

    if (!*dst->array)
    {
        dstr_free(dst);
        return;
    }

    dst->len = len < 0 ? strlen(dst->array) : (size_t)len;
}

/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _UTIL_SYSOBJ_H_
#define _UTIL_SYSOBJ_H_

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <stdint.h>  /* for *int*_t types */
#include <unistd.h>  /* for getuid() */
#include <ctype.h>   /* for isxdigit(), etc. */

gboolean util_have_root();
void util_null_trailing_slash(gchar *str); /* in-place */
void util_compress_space(gchar *str); /* in-place, multiple whitespace replaced by one space */
void util_strstrip_double_quotes_dumb(gchar *str); /* in-place, strips any double-quotes from the start and end of str */
gchar *util_build_fn(const gchar *base, const gchar *name); /* returns "<base>[/<name>]" */
gchar *util_canonicalize_path(const gchar *path);
gchar *util_normalize_path(const gchar *path, const gchar *relto);
gsize util_count_lines(const gchar *str);
gchar *util_escape_markup(gchar *v, gboolean replacing);
int32_t util_get_did(gchar *str, const gchar *lbl); /* ("cpu6", "cpu") -> 6 */
int util_maybe_num(gchar *str); /* returns the guessed base, 0 for not num */
gchar *util_find_line_value(gchar *data, gchar *key, gchar delim);

/* to quiet -Wunused-parameter nagging.  */
#define PARAM_NOT_UNUSED(p) (void)p

/* appends an element to a string, adding a space if
 * the string is not empty.
 * ex: ret = appf(ret, "%s=%s\n", name, value); */
gchar *appf(gchar *src, gchar *fmt, ...);
gchar *appfs(gchar *src, const gchar *sep, gchar *fmt, ...);

#endif

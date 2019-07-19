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

#include "appf.h"

gchar *appf(gchar *src, const gchar *fmt, ...) {
    gchar *buf, *ret;
    va_list args;
    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);
    if (src != NULL) {
        ret = g_strconcat(src, strlen(src) ? " " : "", buf, NULL);
        g_free(buf);
        g_free(src);
    } else
        ret = buf;
    return ret;
}

gchar *appfs(gchar *src, const gchar *sep, const gchar *fmt, ...) {
    gchar *buf, *ret;
    va_list args;
    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);
    if (src != NULL) {
        ret = g_strconcat(src, strlen(src) ? sep : "", buf, NULL);
        g_free(buf);
        g_free(src);
    } else
        ret = buf;
    return ret;
}

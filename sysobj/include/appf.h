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

#ifndef _APPF_H_
#define _APPF_H_

#include <glib.h>

/* appends a formatted element to a string, adding a space (or sep)
 * if the string is not empty. The string is created if src is null.
 * ex: ret = appf(ret, "%s", list_item);
 * ex: ret = appfs(ret, "\n", "%s=%s", name, value); */
gchar *appf(gchar *src, const gchar *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
gchar *appfs(gchar *src, const gchar *sep, const gchar *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));

#endif

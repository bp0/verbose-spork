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

/* appends a formatted element to a string, adding a separator string
 * if the string is not empty. The string is created if src is null.
 * ex: ret = appfs(ret, "; ", "%s = %d", name, value); */
char *appfs(char *src, const char *sep, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));

/* for convenience */
#define appfsp(src, fmt, ...) appfs(src, " ", fmt, __VA_ARGS__)
#define appfnl(src, fmt, ...) appfs(src, "\n", fmt, __VA_ARGS__)

/* preserves the original appf() behavior, where separator was space. */
#define appf(...) appfsp(__VA_ARGS__)

#endif

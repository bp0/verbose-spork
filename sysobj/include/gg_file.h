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

#ifndef _GG_FILE_H_
#define _GG_FILE_H_

#include <glib.h>
#include <sys/errno.h> /* err = errno */

/* why?: `cat /sys/kernel/security/apparmor/revision` seems to block forever */

/* a g_file_get_contents() work-alike that doesn't block (for more than 0.1s), err = errno */
gboolean gg_file_get_contents_non_blocking(const gchar *file, gchar **contents, gsize *size, int *err);

#endif

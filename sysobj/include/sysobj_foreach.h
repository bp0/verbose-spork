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

#ifndef _SYSOBJ_FOREACH_H_
#define _SYSOBJ_FOREACH_H_

#include "sysobj.h"

#define SYSOBJ_FOREACH_STOP FALSE
#define SYSOBJ_FOREACH_CONTINUE TRUE
typedef gboolean (*f_sysobj_foreach)(const sysobj *s, gpointer user_data);
void sysobj_foreach(GSList *filters, f_sysobj_foreach callback, gpointer user_data);

#endif

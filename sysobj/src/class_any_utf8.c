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

#include "sysobj.h"

static sysobj_class *cls_any_utf8;

gboolean any_utf8_verify(sysobj *obj) {
    gboolean was_read = obj->data.was_read;
    gboolean verified = FALSE;
    if (!was_read)
        sysobj_read_data(obj);
    if (obj->data.is_utf8 && obj->data.len)
        verified = TRUE;
    if (!was_read)
        sysobj_unread_data(obj);
    return verified;
}

void class_any_utf8() {
    cls_any_utf8 = class_new();
    cls_any_utf8->tag = "any";
    cls_any_utf8->pattern = "*";
    cls_any_utf8->flags = OF_GLOB_PATTERN;
    cls_any_utf8->f_verify = any_utf8_verify;
    class_add(cls_any_utf8);
}

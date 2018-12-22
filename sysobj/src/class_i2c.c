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
#include "format_funcs.h"

static gboolean i2c_verify(sysobj *obj);

static sysobj_class cls_i2c[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "i2c:adapter", .pattern = "/sys/devices*/i2c-*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = i2c_verify, .f_format = fmt_node_name },
  { SYSOBJ_CLASS_DEF
    .tag = "i2c:dev", .pattern = "/sys/devices*/i2c-dev/i2c-*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = i2c_verify, .f_format = fmt_node_name },
};

static gboolean i2c_verify(sysobj *obj) {
    return verify_lblnum(obj, "i2c-");
}

void class_i2c() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_i2c); i++)
        class_add(&cls_i2c[i]);
}

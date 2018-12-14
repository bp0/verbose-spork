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

static gchar *mobo_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_mobo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "mobo", .pattern = ":/mobo*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = mobo_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
};

static gchar *mobo_format(sysobj *obj, int fmt_opts) {
    if (SEQ(":/mobo", obj->path)) {
        gchar *name = sysobj_raw_from_fn(":/mobo", "name");
        return name;
    }
    return simple_format(obj, fmt_opts);
}

void class_mobo() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_mobo); i++)
        class_add(&cls_mobo[i]);
}

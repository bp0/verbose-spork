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
static double mobo_update_interval(sysobj *obj);

static sysobj_class cls_mobo[] = {
  { .tag = "mobo", .pattern = ":/mobo*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = mobo_format, .f_update_interval = mobo_update_interval },
};

static gchar *mobo_format(sysobj *obj, int fmt_opts) {
    if (!strcmp("mobo", obj->name)) {
        gchar *name = sysobj_raw_from_fn(":/mobo", "name");
        return name;
    }
    return simple_format(obj, fmt_opts);
}

static double mobo_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 0.0;
}

void class_mobo() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_mobo); i++) {
        class_add(&cls_mobo[i]);
    }
}

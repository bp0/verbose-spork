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

static gchar *meminfo_format(sysobj *obj, int fmt_opts);
const gchar *mem_label(sysobj *obj);

static sysobj_class cls_meminfo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "meminfo", .pattern = ":/meminfo", .flags = OF_CONST,
    .f_format = meminfo_format },
  { SYSOBJ_CLASS_DEF
    .tag = "meminfo:stat", .pattern = ":/meminfo/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_label = mem_label, .f_format = meminfo_format, .s_update_interval = 1.0 },
};

static const struct { gchar *item; gchar *lbl; int extra_flags; func_format f_func; } mem_items[] = {
    { "MemTotal", "total memory available", OF_NONE, fmt_KiB_to_higher },
    { NULL, NULL, 0 }
};

int mem_lookup(const gchar *key) {
    for(int i = 0; mem_items[i].item; i++)
        if (SEQ(key, mem_items[i].item))
            return i;
    return -1;
}

const gchar *mem_label(sysobj *obj) {
    int i = mem_lookup(obj->name);
    if (i != -1)
        return _(mem_items[i].lbl);
    return NULL;
}

static gchar *meminfo_format(sysobj *obj, int fmt_opts) {
    int64_t mkb = 0;
    if (SEQ(obj->name, "meminfo") ) {
        return sysobj_format_from_fn(obj->path, "MemTotal", fmt_opts);
    } else if (*obj->name != '_') {
        int i = mem_lookup(obj->name);
        if (i != -1 &&  mem_items[i].f_func)
             return mem_items[i].f_func(obj, fmt_opts);
        return fmt_KiB(obj, fmt_opts);
    }
    return simple_format(obj, fmt_opts);
}

void class_meminfo() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_meminfo); i++)
        class_add(&cls_meminfo[i]);
}

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

attr_tab mem_items[] = {
    { "MemTotal", N_("total memory available"), OF_NONE, fmt_KiB_to_higher },
    { "SwapTotal", N_("total swap space available"), OF_NONE, fmt_KiB_to_higher },
    { "SwapFree", N_("total swap space free"), OF_NONE, fmt_KiB_to_higher },
    { "VmallocTotal", NULL, OF_NONE, fmt_KiB_to_higher },
    ATTR_TAB_LAST
};

static sysobj_class cls_meminfo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "meminfo", .pattern = ":/meminfo", .flags = OF_CONST,
    .f_format = meminfo_format, .s_label = N_("Memory information from /proc/meminfo") },
  { SYSOBJ_CLASS_DEF
    .tag = "meminfo:stat", .pattern = ":/meminfo/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    /* attr_tab not complete, so .f_verify is needed to let everyone in */
    .attributes = mem_items, .f_verify = verify_true,
    .f_format = meminfo_format, .s_update_interval = 1.0 },
};

static gchar *meminfo_format(sysobj *obj, int fmt_opts) {
    int64_t mkb = 0;
    if (SEQ(obj->name, "meminfo") ) {
        return sysobj_format_from_fn(obj->path, "MemTotal", fmt_opts);
    } else if (*obj->name != '_') {
        int i = attr_tab_lookup(mem_items, obj->name);
        if (i != -1 &&  mem_items[i].fmt_func)
             return mem_items[i].fmt_func(obj, fmt_opts);
        return fmt_KiB(obj, fmt_opts);
    }
    return simple_format(obj, fmt_opts);
}

void class_meminfo() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_meminfo); i++)
        class_add(&cls_meminfo[i]);
}

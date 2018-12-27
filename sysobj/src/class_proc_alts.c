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

static const gchar *proc_alts_label(sysobj *s);
static gchar *proc_alts_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_proc_alts[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "proc_use_alt:meminfo", .pattern = "/proc/meminfo", .flags = OF_CONST,
    .s_label = N_("Raw meminfo"), .s_suggest = ":/meminfo",
    .f_format = proc_alts_format, .s_update_interval = 30.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "proc_use_alt:cpuinfo", .pattern = "/proc/cpuinfo", .flags = OF_CONST,
    .s_label = N_("Raw cpuinfo"), .s_suggest = ":/cpu/cpuinfo",
    .f_format = proc_alts_format, .s_update_interval = 30.0 },
};

static gchar *proc_alts_format(sysobj *obj, int fmt_opts) {
    return simple_format(obj, fmt_opts);
}

void class_proc_alts() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_proc_alts); i++)
        class_add(&cls_proc_alts[i]);
}

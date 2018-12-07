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

static gchar *meminfo_format(sysobj *obj, int fmt_opts);
static double meminfo_update_interval(sysobj *obj);

static sysobj_class cls_meminfo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "meminfo", .pattern = ":/meminfo*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = meminfo_format, .f_update_interval = meminfo_update_interval },
};

static gchar *format_KMGT_B(double v /*in K*/, gboolean stayk, int fmt_opts) {
    /* compared to KB */
    static const double TiB = 1024*1024*1024;
    static const double GiB = 1024*1024;
    static const double MiB = 1024;
    gboolean show_unit = !(fmt_opts & FMT_OPT_NO_UNIT);
    if (!stayk && v > 2*TiB)
        return g_strdup_printf("%0.1f %s", v / TiB, show_unit ? _("TiB") : "");
    if (!stayk && v > 2*GiB)
        return g_strdup_printf("%0.1f %s", v / GiB, show_unit ? _("GiB") : "");
    if (!stayk && v > 2*MiB)
        return g_strdup_printf("%0.1f %s", v / MiB, show_unit ? _("MiB") : "");

    return g_strdup_printf("%0.1f %s", v, show_unit ? _("KiB") : "");
}

/* note: frees v_str */
int64_t convert_kb_mem_str(gchar *v_str) {
    int64_t n = 0;
    if (!v_str) return -1;
    /* "NNNNN kB" -> NNNNN */
    int mc = sscanf(v_str, "%ld kB", &n);
    if (mc == 1)
        return n;
    return -1;
}

static gchar *meminfo_format(sysobj *obj, int fmt_opts) {
    int64_t mkb = 0;
    if (!strcmp(obj->name, "meminfo") ) {
        gchar *mkb_str = sysobj_raw_from_fn(obj->path, "MemTotal");
        mkb = convert_kb_mem_str(mkb_str);
        g_free(mkb_str);
        if (mkb >= 0)
            return format_KMGT_B((double)mkb, FALSE, fmt_opts);
    } else if (*obj->name != '_') {
        mkb = convert_kb_mem_str(obj->data.str);
        return format_KMGT_B((double)mkb, TRUE, fmt_opts);
    }
    return simple_format(obj, fmt_opts);
}

static double meminfo_update_interval(sysobj *obj) {
    if (!strcmp(obj->path, ":/meminfo") )
        return 3.0;
    return 1.0;
}

void class_meminfo() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_meminfo); i++) {
        class_add(&cls_meminfo[i]);
    }
}

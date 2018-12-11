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

static gchar *uptime_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_uptime[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "uptime", .pattern = "/proc/uptime", .flags = OF_CONST,
    .s_label = N_("System Up/Idle time"),
    .f_format = uptime_format, .s_update_interval = 1.0 },
};

static gchar *uptime_format(sysobj *obj, int fmt_opts) {
    double up = 0, idle = 0;
    int mc = sscanf(obj->data.str, "%lf %lf", &up, &idle);
    if (mc > 0) {
        if (fmt_opts & FMT_OPT_NO_UNIT
            || fmt_opts & FMT_OPT_NO_TRANSLATE)
            return g_strdup_printf("%lf", up);

        if (fmt_opts & FMT_OPT_PART
            || fmt_opts & FMT_OPT_SHORT)
            return formatted_time_span(up, TRUE, TRUE);

        gchar *up_str = formatted_time_span(up, FALSE, TRUE);
        if (idle && fmt_opts & FMT_OPT_COMPLETE) {
            gchar *idle_str = formatted_time_span(idle, FALSE, TRUE);
            gchar *ret = g_strdup_printf("Uptime: %s\nTime spent idle (all CPU): %s", up_str, idle_str);
            g_free(up_str);
            g_free(idle_str);
            return ret;
        } else
            return up_str;
    }
    return simple_format(obj, fmt_opts);
}

void class_uptime() {
    /* add class(es) */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_uptime); i++)
        class_add(&cls_uptime[i]);
}

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

static const gchar *uptime_label(sysobj *s);
static gchar *uptime_format(sysobj *obj, int fmt_opts);
static double uptime_update_interval(sysobj *obj);

static sysobj_class cls_uptime[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "uptime", .pattern = "/proc/uptime", .flags = OF_CONST,
    .s_label = N_("System Up/Idle time"),
    .f_format = uptime_format, .f_update_interval = uptime_update_interval },
};

static gchar *formatted_time_span(double real_seconds, gboolean short_version, gboolean include_seconds) {
    long days = 0, hours = 0, minutes = 0, seconds = 0;

    seconds = real_seconds;
    minutes = seconds / 60; seconds %= 60;
    hours = minutes / 60; minutes %= 60;
    days = hours / 24; hours %= 24;

    const gchar *days_fmt, *hours_fmt, *minutes_fmt, *seconds_fmt;
    const gchar *sep = " ";
    gchar *ret = NULL;

    if (short_version) {
        days_fmt = ngettext("%dd", "%dd", days);
        hours_fmt = ngettext("%dh", "%dh", hours);
        minutes_fmt = ngettext("%dm", "%dm", minutes);
        seconds_fmt = ngettext("%ds", "%ds", seconds);
        sep = ":";
    } else {
        days_fmt = ngettext("%d day", "%d days", days);
        hours_fmt = ngettext("%d hour", "%d hours", hours);
        minutes_fmt = ngettext("%d minute", "%d minutes", minutes);
        seconds_fmt = ngettext("%d second", "%d seconds", seconds);
    }

    if (days > 1)
        ret = appfs(ret, sep, days_fmt, days);
    if (ret || hours > 1)
        ret = appfs(ret, sep, hours_fmt, hours);
    if (ret || minutes > 1 || !include_seconds)
        ret = appfs(ret, sep, minutes_fmt, minutes);
    if (include_seconds)
        ret = appfs(ret, sep, seconds_fmt, seconds);

    return ret;
}

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

static double uptime_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 1.0; /* although, display is in minutes */
}

void class_uptime() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_uptime); i++) {
        class_add(&cls_uptime[i]);
    }
}

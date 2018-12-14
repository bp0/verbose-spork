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

#include <stdlib.h>
#include "format_funcs.h"

#define CHECK_OBJ()  \
    if ( !(obj && obj->data.was_read && obj->data.str) ) \
        return simple_format(obj, fmt_opts);
#define PREP_RAW() \
    gchar *raw = g_strstrip(g_strdup(obj->data.str));    \
    if (fmt_opts & FMT_OPT_NO_UNIT                       \
        && (!fmt_opts & FMT_OPT_PART) )                  \
        return raw;
#define PREP_RAW_RJ(reject) \
    gchar *raw = g_strstrip(g_strdup(obj->data.str));    \
    if (fmt_opts & FMT_OPT_NO_UNIT                       \
        && (!fmt_opts & FMT_OPT_PART) )                  \
        return raw;                                      \
    if (fmt_opts & reject)                               \
        return raw;
#define FINISH_RAW() g_free(raw);

gchar *fmt_nanoseconds(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double ns = strtod(raw, NULL);
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.1lf", ns)
        : g_strdup_printf("%.1lf %s", ns, _("ns"));
}

gchar *fmt_khz_to_mhz(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mhz = strtod(raw, NULL);
    mhz /= 1000; /* raw is khz */
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.3f", mhz)
        : g_strdup_printf("%.3f %s", mhz, _("MHz"));
}

gchar *fmt_mhz(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mhz = strtod(raw, NULL);
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.3f", mhz)
        : g_strdup_printf("%.3f %s", mhz, _("MHz"));
}

gchar *fmt_millidegree_c(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double degc = strtod(raw, NULL);
    degc /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", degc, _("\u00B0C") );
}

gchar *fmt_milliampere(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mA = strtod(raw, NULL);
    FINISH_RAW();
    if (mA > 2000)
        return g_strdup_printf("%.3lf %s", (mA/1000), _("A") );
    else
        return g_strdup_printf("%.1lf %s", mA, _("mA") );
}

gchar *fmt_microwatt(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mW = strtod(raw, NULL);
    mW /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", mW, _("mW") );
}

gchar *fmt_milliseconds(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double ms = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("%.1lf %s", ms, _("ms") );
}

gchar *fmt_microjoule(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mJ = strtod(raw, NULL);
    mJ /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", mJ, _("mJ") );
}

gchar *fmt_percent(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double perc = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("%.1lf%s", perc, _("%") );
}

gchar *fmt_millepercent(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double perc = strtod(raw, NULL);
    perc /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf%s", perc, _("%") );
}

gchar *fmt_hz_to_mhz(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mhz = strtod(raw, NULL);
    mhz /= 1000000;
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.3f", mhz)
        : g_strdup_printf("%.3f %s", mhz, _("MHz"));
}

gchar *fmt_hz(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double hz = strtod(raw, NULL);
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.1f", hz)
        : g_strdup_printf("%.1f %s", hz, _("Hz"));
}

gchar *fmt_millivolt(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double volt = strtod(raw, NULL);
    volt /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", volt, _("V") );
}

gchar *fmt_rpm(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double rpm = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("%.1lf %s", rpm, _("RPM") );
}

gchar *fmt_1yes0no(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    int value = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("[%d] %s", value, value ? _("Yes") : _("No") );
}

gchar *fmt_KiB(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double v = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("%0.1f %s", v, _("KiB") );
}

static const double bytes_KiB = 1024.0;
static const double bytes_MiB = bytes_KiB * 1024.0;
static const double bytes_GiB = bytes_MiB * 1024.0;
static const double bytes_TiB = bytes_GiB * 1024.0;
static const double bytes_PiB = bytes_TiB * 1024.0;

gchar *fmt_KiB_to_MiB(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double v = strtod(raw, NULL);
    v *= bytes_KiB; /* v is KiB */
    FINISH_RAW();

    if (v > 2 * bytes_MiB)
        return g_strdup_printf("%0.3f %s", v / bytes_MiB, _("MiB") );

    return g_strdup_printf("%0.1f %s", v / bytes_KiB, _("KiB") );
}

gchar *fmt_KiB_to_higher(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double v = strtod(raw, NULL);
    v *= bytes_KiB; /* v is KiB */
    FINISH_RAW();

    if (v > 2 * bytes_PiB)
        return g_strdup_printf("%0.1f %s", v / bytes_PiB, _("PiB") );
    if (v > 2 * bytes_TiB)
        return g_strdup_printf("%0.1f %s", v / bytes_TiB, _("TiB") );
    if (v > 2 * bytes_GiB)
        return g_strdup_printf("%0.1f %s", v / bytes_GiB, _("GiB") );
    if (v > 2 * bytes_MiB)
        return g_strdup_printf("%0.1f %s", v / bytes_MiB, _("MiB") );

    return g_strdup_printf("%0.1f %s", v / bytes_KiB, _("KiB") );
}

gchar *fmt_seconds(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double seconds = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", seconds, _("s"));
}

gchar *fmt_seconds_to_span(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW_RJ(FMT_OPT_NO_TRANSLATE);
    double seconds = strtod(raw, NULL);
    FINISH_RAW();
    if (fmt_opts & FMT_OPT_PART
        || fmt_opts & FMT_OPT_SHORT)
        return formatted_time_span(seconds, TRUE, TRUE); /* short */
    return formatted_time_span(seconds, FALSE, TRUE);    /* long */
}

gchar *formatted_time_span(double real_seconds, gboolean short_version, gboolean include_seconds) {
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

/* table is null-terminated string list of null-terminated UNTRANSLATED strings
 * table_len, optional, improves speed
 * nbase is the base of the number stored in obj->data.str
 * fmt_opts, as usual */
gchar *sysobj_format_table(sysobj *obj, gchar **table, int table_len, int nbase, int fmt_opts) {
    gchar *val = NULL;
    int index = 0, count = 0;

    FMT_OPTS_IGNORE(); //TODO:

    if (obj && obj->data.str && isxdigit(*(obj->data.str)) ) {
        index = strtol(obj->data.str, NULL, nbase);
        if (table_len) {
            if (index < table_len)
                val = table[index];
        } else {
            gchar **p = table;
            while(p) {
                if (count == index) {
                    val = *p;
                    break;
                }
                count++;
                p++;
            }
        }
        if (val)
            return g_strdup_printf("[%d] %s", index, _(val));
        return g_strdup_printf("[%d]", index);
    }
    return simple_format(obj, fmt_opts);
}

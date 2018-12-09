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

#include "format_funcs.h"

#define CHECK_OBJ()  \
    if ( !(obj && obj->data.was_read && obj->data.str) ) \
        return simple_format(obj, fmt_opts);
#define PREP_RAW() \
    gchar *raw = g_strstrip(g_strdup(obj->data.str));    \
    if (fmt_opts & FMT_OPT_NO_UNIT)                      \
        return raw;
#define FINISH_RAW() g_free(raw);

gchar *fmt_nanoseconds(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    uint32_t ns = strtoul(raw, NULL, 10);
    FINISH_RAW();
    return g_strdup_printf("%u %s", ns, _("ns"));
}

gchar *fmt_khz(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mhz = strtoul(raw, NULL, 10);
    mhz /= 1000; /* raw is khz */
    FINISH_RAW();
    return g_strdup_printf("%.3f %s", mhz, _("MHz"));
}

gchar *fmt_millidegree_c(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double degc = strtol(raw, NULL, 10);
    degc /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", degc, _("\u00B0C") );
}

gchar *fmt_milliampere(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mA = strtol(raw, NULL, 10);
    FINISH_RAW();
    if (mA > 2000)
        return g_strdup_printf("%.3lf %s", (mA/1000), _("A") );
    else
        return g_strdup_printf("%.1lf %s", mA, _("mA") );
}

gchar *fmt_microwatt(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mW = strtol(raw, NULL, 10);
    mW /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", mW, _("mW") );
}

gchar *fmt_milliseconds(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double ms = strtol(raw, NULL, 10);
    FINISH_RAW();
    return g_strdup_printf("%.1lf %s", ms, _("ms") );
}

gchar *fmt_microjoule(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mJ = strtol(raw, NULL, 10);
    mJ /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", mJ, _("mJ") );
}

gchar *fmt_percent(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double perc = strtol(raw, NULL, 10);
    FINISH_RAW();
    return g_strdup_printf("%.1lf%s", perc, _("%") );
}

gchar *fmt_millepercent(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double perc = strtol(raw, NULL, 10);
    perc /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf%s", perc, _("%") );
}

gchar *fmt_hz(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double hz = strtol(raw, NULL, 10);
    FINISH_RAW();
    return g_strdup_printf("%.1lf %s", hz, _("Hz") );
}

gchar *fmt_millivolt(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double volt = strtol(raw, NULL, 10);
    volt /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.3lf %s", volt, _("V") );
}

gchar *fmt_rpm(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double rpm = strtol(raw, NULL, 10);
    FINISH_RAW();
    return g_strdup_printf("%.1lf %s", rpm, _("RPM") );
}

gchar *fmt_1yes0no(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    int value = strtol(raw, NULL, 10);
    FINISH_RAW();
    return g_strdup_printf("[%d] %s", value, value ? _("Yes") : _("No") );
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

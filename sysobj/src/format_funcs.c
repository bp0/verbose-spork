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

gchar *fmt_microseconds_to_milliseconds(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double ms = strtod(raw, NULL);
    ms /= 1000;
    FINISH_RAW();
    return g_strdup_printf("%.1lf %s", ms, _("ms") );
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

gchar *fmt_megabitspersecond(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mbps = strtod(raw, NULL);
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.1f", mbps)
        : g_strdup_printf("%.1f %s", mbps, _("Mbps"));
}

gchar *fmt_KiB(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double v = strtod(raw, NULL);
    FINISH_RAW();
    return g_strdup_printf("%0.1f %s", v, _("KiB") );
}

#define bytes_KiB (1024.0)
#define bytes_MiB (bytes_KiB * 1024.0)
#define bytes_GiB (bytes_MiB * 1024.0)
#define bytes_TiB (bytes_GiB * 1024.0)
#define bytes_PiB (bytes_TiB * 1024.0)

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

gchar *fmt_megatransferspersecond(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mbps = strtod(raw, NULL);
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.1f", mbps)
        : g_strdup_printf("%.1f %s", mbps, _("MT/s"));
}

gchar *fmt_gigatransferspersecond(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double mbps = strtod(raw, NULL);
    FINISH_RAW();
    return fmt_opts & FMT_OPT_NO_UNIT
        ? g_strdup_printf("%.1f", mbps)
        : g_strdup_printf("%.1f %s", mbps, _("GT/s"));
}

gchar *fmt_lanes_x(sysobj *obj, int fmt_opts) {
    CHECK_OBJ();
    PREP_RAW();
    double lanes = strtod(raw, NULL);
    FINISH_RAW();
    return util_strchomp_float(g_strdup_printf("x%.0lf", lanes));
}

gchar *fmt_node_name(sysobj *obj, int fmt_opts) {
    if (obj && obj->data.is_dir) {
        gchar *name = sysobj_format_from_fn(obj->path, "name", fmt_opts);
        if (name)
            return name;
    }
    return simple_format(obj, fmt_opts);
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

gchar *sysobj_format_lookup_tab(sysobj *obj, lookup_tab *tab, int fmt_opts) {
    gchar *ret = NULL;
    gchar *val = g_strstrip(g_strdup(obj->data.str));
    for (int i = 0; tab[i].value; i++) {
        if (SEQ(val, tab[i].value)) {
            if (fmt_opts & FMT_OPT_SHORT || fmt_opts & FMT_OPT_PART) {
                if (tab[i].s_def_short)
                    ret = g_strdup(_(tab[i].s_def_short));
                else
                    ret = g_strdup_printf("%s", val);
            } else {
                if (tab[i].s_def)
                    ret = g_strdup_printf("[%s] %s", val, _(tab[i].s_def) );
                else
                    ret = g_strdup_printf("[%s]", val);
            }
        }
    }
    return ret;
}

const gchar *color_lookup(const gchar *ansi_color) {
    static struct { char *ansi, *html; } tab[] = {
        { "0;30", "#010101" },
        { "0;31", "#de382b" },
        { "0;32", "#39b54a" },
        { "0;33", "#ffc706" },
        { "0;34", "#006fb8" },
        { "0;35", "#762671" },
        { "0;36", "#2cb5e9" },
        { "0;37", "#cccccc" },
        { "1;30", "#808080" },
        { "1;31", "#ff0000" },
        { "1;32", "#00ff00" },
        { "1;33", "#ffff00" },
        { "1;34", "#0000ff" },
        { "1;35", "#ff00ff" },
        { "1;36", "#00ffff" },
        { "1;37", "#ffffff" },
    };
    gchar tmp[5] = "    ";
    strncpy(tmp, ansi_color, 4);
    for (int i = 0; i<(int)G_N_ELEMENTS(tab); i++)
        if (SEQ(tab[i].ansi, tmp) )
            return tab[i].html;
    return NULL;
}

const gchar *color_lookup_bg(const gchar *ansi_color) {
    static struct { char *ansi, *html; } tab[] = {
        { "0;40", "#010101" },
        { "0;41", "#de382b" },
        { "0;42", "#39b54a" },
        { "0;43", "#ffc706" },
        { "0;44", "#006fb8" },
        { "0;45", "#762671" },
        { "0;46", "#2cb5e9" },
        { "0;47", "#cccccc" },
        { "1;40", "#808080" },
        { "1;41", "#ff0000" },
        { "1;42", "#00ff00" },
        { "1;43", "#ffff00" },
        { "1;44", "#0000ff" },
        { "1;45", "#ff00ff" },
        { "1;46", "#00ffff" },
        { "1;47", "#ffffff" },
    };
    gchar *chk = strchr(ansi_color, '|');
    if (!chk) return tab[0].html;
    for (int i = 0; i<(int)G_N_ELEMENTS(tab); i++)
        if (SEQ(tab[i].ansi, chk + 1) )
            return tab[i].html;
    return NULL;
}

gchar *safe_ansi_color(gchar *ansi_color, gboolean free_in) {
    if (!ansi_color) return NULL;
    int c1 = 0, c2 = 0;
    int mc = sscanf(ansi_color, "%d;%d", &c1, &c2);
    if (free_in)
        g_free(ansi_color);
    if (mc > 1) {
        if (mc == 1) {
            c2 = c1;
            c1 = 0;
        }
        if (c1 == 0 || c1 == 1) {
            if (c2 >= 90 && c2 <= 97) {
                c1 = 1;
                c2 -= 60;
            }
            if (c2 >= 30 && c2 <= 37)
                return g_strdup_printf("%d;%d", c1, c2);
        }
    }
    return NULL;
}

gchar *safe_ansi_color_ex(gchar *ansi_color, gboolean free_in) {
    if (!ansi_color) return NULL;
    int c1 = 0, c2 = 0, c3 = 0, c4 = 0, ok = 0;
    int mc = sscanf(ansi_color, "%d;%d|%d;%d", &c1, &c2, &c3, &c4);
    if (free_in)
        g_free(ansi_color);
    if (mc > 2) {
        if (mc == 3) {
            c4 = c3;
            c3 = 0;
        }
        if (c3 == 0 || c3 == 1) {
            if (c4 >= 100 && c4 <= 107) {
                c3 = 1;
                c4 -= 60;
            }
            if (c4 >= 40 && c4 <= 47)
                ok += 2;
        }
    }
    if (mc > 1) {
        if (mc == 1) {
            c2 = c1;
            c1 = 0;
        }
        if (c1 == 0 || c1 == 1) {
            if (c2 >= 90 && c2 <= 97) {
                c1 = 1;
                c2 -= 60;
            }
            if (c2 >= 30 && c2 <= 37)
                ok += 1;
        }
    }
    if (ok == 1)
        return g_strdup_printf("%d;%d", c1, c2);
    if (ok == 3)
        return g_strdup_printf("%d;%d|%d;%d", c1, c2, c3, c4);
    return NULL;
}

gchar *format_with_ansi_color(const gchar *str, const gchar *ansi_color, int fmt_opts) {
    gchar *ret = NULL;
    gchar *safe_color = g_strdup(ansi_color);
    util_strstrip_double_quotes_dumb(safe_color);
    safe_color = safe_ansi_color_ex(safe_color, TRUE);
    if (!safe_color)
        goto format_with_ansi_color_end;

    const gchar *html_color_fg = color_lookup(safe_color);
    const gchar *html_color_bg = color_lookup_bg(safe_color);

    if (fmt_opts & FMT_OPT_ATERM) {
        gchar *bg = strchr(safe_color, '|');
        if (bg) {
            *bg = 0;
            ret = g_strdup_printf("\x1b[%sm\x1b[%sm%s" ANSI_COLOR_RESET, safe_color, bg + 1, str);
        } else
            ret = g_strdup_printf("\x1b[%sm%s" ANSI_COLOR_RESET, safe_color, str);
    } else if (fmt_opts & FMT_OPT_PANGO)
        ret = g_strdup_printf("<span background=\"%s\" color=\"%s\"><b> %s </b></span>", html_color_bg, html_color_fg, str);
    else if (fmt_opts & FMT_OPT_HTML)
        ret = g_strdup_printf("<span style=\"background-color: %s; color: %s;\"><b>&nbsp;%s&nbsp;</b></span>", html_color_bg, html_color_fg, str);

format_with_ansi_color_end:
    g_free(safe_color);
    if (!ret)
        ret = g_strdup(str);
    return ret;
}

void tag_vendor(gchar **str, guint offset, const gchar *vendor_str, const char *ansi_color, int fmt_opts) {
    if (!str || !*str) return;
    if (!vendor_str || !ansi_color) return;
    gchar *work = *str, *new = NULL;
    if (g_str_has_prefix(work + offset, vendor_str)
        || strncasecmp(work + offset, vendor_str, strlen(vendor_str)) == 0) {
        gchar *cvs = format_with_ansi_color(vendor_str, ansi_color, fmt_opts);
        *(work+offset) = 0;
        new = g_strdup_printf("%s%s%s", work, cvs, work + offset + strlen(vendor_str) );
        g_free(work);
        *str = new;
        g_free(cvs);
    }
}

gchar *vendor_match_tag(const gchar *vendor_str, int fmt_opts) {
    const Vendor *v = vendor_match(vendor_str, NULL);
    if (v) {
        gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
        tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, fmt_opts);
        return ven_tag;
    }
    return NULL;
}

#define STD_FORMAT_FUNC(f) { (gpointer)f ,  #f },
fmt_func_tab format_funcs[] = {
STD_FORMAT_FUNC(fmt_nanoseconds)
STD_FORMAT_FUNC(fmt_milliseconds)
STD_FORMAT_FUNC(fmt_microseconds_to_milliseconds)
STD_FORMAT_FUNC(fmt_seconds)
STD_FORMAT_FUNC(fmt_seconds_to_span)
STD_FORMAT_FUNC(fmt_hz)
STD_FORMAT_FUNC(fmt_hz_to_mhz)
STD_FORMAT_FUNC(fmt_khz_to_mhz)
STD_FORMAT_FUNC(fmt_mhz)
STD_FORMAT_FUNC(fmt_millidegree_c)
STD_FORMAT_FUNC(fmt_milliampere)
STD_FORMAT_FUNC(fmt_microwatt)
STD_FORMAT_FUNC(fmt_microjoule)
STD_FORMAT_FUNC(fmt_percent)
STD_FORMAT_FUNC(fmt_millepercent)
STD_FORMAT_FUNC(fmt_millivolt)
STD_FORMAT_FUNC(fmt_rpm)
STD_FORMAT_FUNC(fmt_1yes0no)
STD_FORMAT_FUNC(fmt_megabitspersecond)
STD_FORMAT_FUNC(fmt_KiB)
STD_FORMAT_FUNC(fmt_KiB_to_MiB)
STD_FORMAT_FUNC(fmt_KiB_to_higher)
STD_FORMAT_FUNC(fmt_megatransferspersecond)
STD_FORMAT_FUNC(fmt_gigatransferspersecond)
STD_FORMAT_FUNC(fmt_lanes_x)
STD_FORMAT_FUNC(fmt_node_name)
};

const gchar *format_funcs_lookup(const gpointer fp) {
    for(int i = 0; i<(int)G_N_ELEMENTS(format_funcs); i++)
        if (format_funcs[i].func == fp)
            return format_funcs[i].name;
    return NULL;
}

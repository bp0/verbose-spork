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

#ifndef _FORMAT_FUNCS_H_
#define _FORMAT_FUNCS_H_

#include "sysobj.h"

/* the input unit is specified, but the formatted
 * output maybe be some other unit
 * ex: fmt_khz() the input must be khz, but it may format as MHz.
 *
 * fmt_opts:
 * FMT_OPT_NO_UNIT : do not allow conversion
 * FMT_OPT_PART | FMT_OPT_NO_UNIT : allow conversion, but don't include the unit
 */

gchar *fmt_nanoseconds(sysobj *obj, int fmt_opts);
gchar *fmt_milliseconds(sysobj *obj, int fmt_opts);
gchar *fmt_microseconds_to_milliseconds(sysobj *obj, int fmt_opts);
gchar *fmt_seconds(sysobj *obj, int fmt_opts);
gchar *fmt_seconds_to_span(sysobj *obj, int fmt_opts); /* [D days, ][H hours, ][M minutes, ]S seconds */
gchar *fmt_hz(sysobj *obj, int fmt_opts);
gchar *fmt_hz_to_mhz(sysobj *obj, int fmt_opts);
gchar *fmt_khz_to_mhz(sysobj *obj, int fmt_opts);
gchar *fmt_mhz(sysobj *obj, int fmt_opts);
gchar *fmt_millidegree_c(sysobj *obj, int fmt_opts);
gchar *fmt_milliampere(sysobj *obj, int fmt_opts);
gchar *fmt_microwatt(sysobj *obj, int fmt_opts);
gchar *fmt_microjoule(sysobj *obj, int fmt_opts);
gchar *fmt_percent(sysobj *obj, int fmt_opts);
gchar *fmt_millepercent(sysobj *obj, int fmt_opts);
gchar *fmt_millivolt(sysobj *obj, int fmt_opts);
gchar *fmt_rpm(sysobj *obj, int fmt_opts);
gchar *fmt_1yes0no(sysobj *obj, int fmt_opts);
gchar *fmt_megabitspersecond(sysobj *obj, int fmt_opts);
gchar *fmt_bytes(sysobj *obj, int fmt_opts);
gchar *fmt_bytes_to_higher(sysobj *obj, int fmt_opts);
gchar *fmt_KiB(sysobj *obj, int fmt_opts);
gchar *fmt_KiB_to_MiB(sysobj *obj, int fmt_opts);
gchar *fmt_KiB_to_higher(sysobj *obj, int fmt_opts); /* up to MiB, GiB, etc */
gchar *fmt_megatransferspersecond(sysobj *obj, int fmt_opts);
gchar *fmt_gigatransferspersecond(sysobj *obj, int fmt_opts);
gchar *fmt_lanes_x(sysobj *obj, int fmt_opts);
/* returns formatted <self>/name */
gchar *fmt_node_name(sysobj *obj, int fmt_opts);

/* {{child}}{{sep|child}} */
gchar *format_node_fmt_str(sysobj *obj, int fmt_opts, const gchar *comp_str);

gchar *safe_ansi_color(gchar *ansi_color, gboolean free_in); /* verify the ansi color */
const gchar *color_lookup(int ansi_color); /* ansi_color to html color */
/* wrap the input str with color based on fmt_opts (none,term,html,pango) */
gchar *format_with_ansi_color(const gchar *str, const gchar *ansi_color, int fmt_opts);
gchar *format_as_junk_value(const gchar *str, int fmt_opts);
gchar *formatted_time_span(double real_seconds, gboolean short_version, gboolean include_seconds);

/* table is null-terminated string list of null-terminated UNTRANSLATED strings
 * table_len, optional, improves speed
 * nbase is the base of the number stored in obj->data.str
 * fmt_opts, as usual */
gchar *sysobj_format_table(sysobj *obj, gchar **table, int table_len, int nbase, int fmt_opts);

typedef const struct {
    const gchar *value;
    const gchar *s_def; /* N_() */
    const gchar *s_def_short; /* N_() */
} lookup_tab;
#define LOOKUP_TAB_LAST { NULL, NULL }
gchar *sysobj_format_lookup_tab(sysobj *obj, lookup_tab *tab, int fmt_opts);

void tag_vendor(gchar **str, guint offset, const gchar *vendor_str, const char *ansi_color, int fmt_opts);
gchar *vendor_match_tag(const gchar *vendor_str, int fmt_opts);
gchar *vendor_list_ribbon(const vendor_list vl_in, int fmt_opts);

typedef const struct {
    const gpointer func;
    const gchar *name;
} fmt_func_tab;

extern fmt_func_tab format_funcs[];
const gchar *format_funcs_lookup(const gpointer fp);

#endif

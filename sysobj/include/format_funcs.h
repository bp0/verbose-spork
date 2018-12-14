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
 * ex: fmt_khz() the input must be khz, but it may format as MHz. */

gchar *fmt_nanoseconds(sysobj *obj, int fmt_opts);
gchar *fmt_milliseconds(sysobj *obj, int fmt_opts);
gchar *fmt_seconds(sysobj *obj, int fmt_opts);
gchar *fmt_seconds_to_span(sysobj *obj, int fmt_opts); /* [D days, ][H hours, ][M minutes, ]S seconds */
gchar *fmt_hz(sysobj *obj, int fmt_opts);
gchar *fmt_hz_to_mhz(sysobj *obj, int fmt_opts);
gchar *fmt_khz(sysobj *obj, int fmt_opts);
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
gchar *fmt_KiB(sysobj *obj, int fmt_opts);
gchar *fmt_KiB_to_MiB(sysobj *obj, int fmt_opts);
gchar *fmt_KiB_to_higher(sysobj *obj, int fmt_opts); /* up to MiB, GiB, etc */

gchar *formatted_time_span(double real_seconds, gboolean short_version, gboolean include_seconds);

/* table is null-terminated string list of null-terminated UNTRANSLATED strings
 * table_len, optional, improves speed
 * nbase is the base of the number stored in obj->data.str
 * fmt_opts, as usual */
gchar *sysobj_format_table(sysobj *obj, gchar **table, int table_len, int nbase, int fmt_opts);

#endif

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

#ifndef _SYSOBJ_EXTRAS_H_
#define _SYSOBJ_EXTRAS_H_

#include "vendor.h"

/* in gen_procs.c */
/* obj points to a cpuN/topology */
void cpu_pct(sysobj *obj, int *logical, int *pack, int *core_of_pack, int *thread_of_core);

/* in util_dt.c */
gchar *dtr_compat_decode(const gchar *compat_str_list, gsize len, gboolean show_class);
gchar *dtr_get_opp_kv(const gchar *path, const gchar *key_prefix);

/* in class_cpu.c */
gboolean cpu_verify_child(sysobj *obj);

/* in class_hwmon.c */
gchar *hwmon_attr_encode_name(const gchar *type, int index, const gchar *attrib);
gboolean hwmon_attr_decode_name(const gchar *name, gchar **type, int *index, gchar **attrib, gboolean *is_value);

/* in gen_gpu.c */
void sysobj_virt_add_vendor_match(gchar *base, gchar *name, const Vendor *vendor);

/* in class_gpu.c */
/* replaces the extra chars with spaces, then when done with a series of
 * str_shorten()s, use util_compress_space() to squeeze. */
gboolean str_shorten(gchar *str, const gchar *find, const gchar *replace);

/* in class_mobo.c */
void tag_vendor(gchar **str, guint offset, const gchar *vendor_str, const char *ansi_color, int fmt_opts);

#endif

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

#ifndef _UTIL_DT_H
#define _UTIL_DT_H

#include "sysobj.h"
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h> /* for PRIu64 */
#include <endian.h>

#define DTROOT "/sys/firmware/devicetree/base"

gchar *dtr_compat_decode(const gchar *compat_str_list, gsize len, gboolean show_class);
gchar *dtr_get_opp_kv(const gchar *path, const gchar *key_prefix);

/* operating-points v0,v1,v2 */
typedef struct {
    uint32_t version; /* opp version, 0 = clock-frequency only */
    uint32_t phandle; /* v2 only */
    uint32_t khz_min;
    uint32_t khz_max;
    uint32_t clock_latency_ns;
    char ref_path[128];
} dt_opp_range;

/* free result with g_free() */
dt_opp_range *dtr_get_opp_range(const char *path);

#define DTROOT "/sys/firmware/devicetree/base"

typedef uint32_t dt_uint; /* big-endian */
typedef uint64_t dt_uint64; /* big-endian */

#define UMIN(a,b) MIN(((uint32_t)(a)), ((uint32_t)(b)))

const char *dtr_phandle_lookup(uint32_t v);
const char *dtr_alias_lookup_by_path(const gchar* path);
const char *dtr_symbol_lookup_by_path(const gchar* path);

gchar *dtr_format(sysobj *obj, int fmt_opts);

gboolean dtr_init();
void dtr_cleanup();

extern gchar *dtr_log;

#endif

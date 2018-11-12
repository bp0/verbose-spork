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

/* Generator turns /proc/meminfo into a sysobj tree
 *  :/meminfo
 */

#include "sysobj.h"

#define PROC_MEMINFO "/proc/meminfo"

static int meminfo_was_found = 0;
static gchar *meminfo_found(const gchar *path) {
    PARAM_NOT_UNUSED(path);
    return g_strdup(meminfo_was_found ? "1" : "0");
}

static sysobj_virt vol[] = {
    { .path = ":/meminfo/_found", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = meminfo_found, .f_get_type = NULL },
    { .path = ":/meminfo", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

gchar *meminfo_read(const gchar *path) {
    gchar *ret = NULL;
    gchar *data = sysobj_raw_from_fn(PROC_MEMINFO, NULL);
    gchar *name = g_path_get_basename(path);
    if (data && name) {
        g_strchomp(name);
        ret = util_find_line_value(data, name, ':');
    }
    g_free(data);
    g_free(name);
    return ret;
}

void meminfo_scan() {
    gchar *data = sysobj_raw_from_fn(PROC_MEMINFO, NULL);
    gchar **lines = g_strsplit(data, "\n", -1);
    for(int i = 0; lines[i]; i++) {
        gchar *c = strchr(lines[i], ':');
        if (c) {
            *c = 0;
            g_strchomp(lines[i]);
            sysobj_virt *vo = sysobj_virt_new();
            vo->path = g_strdup_printf(":/meminfo/%s", lines[i]);
            vo->f_get_data = meminfo_read;
            sysobj_virt_add(vo);
        }
    }
    g_strfreev(lines);
    g_free(data);
}

void gen_meminfo() {
    int i = 0;

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    if (sysobj_exists_from_fn(PROC_MEMINFO, NULL)) {
        meminfo_was_found = 1;
        meminfo_scan();
    }
}

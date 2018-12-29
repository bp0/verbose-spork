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
#include "gg_file.h"

#define PROC_MEMINFO "/proc/meminfo"

static gchar *meminfo_path_fs = NULL;
static gchar *meminfo_scan(const gchar *path);
static gchar *meminfo_read(const gchar *path);

static sysobj_virt vol[] = {
    { .path = ":/meminfo", .str = "*",
      .f_get_data = meminfo_scan,
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST | VSO_TYPE_CLEANUP },
};

static gchar *meminfo_scan(const gchar *path) {
    if (!path) {
        /* cleanup */
        g_free(meminfo_path_fs);
        meminfo_path_fs = NULL;
        return NULL;
    }

    if (!meminfo_path_fs) {
        /* init */
        sysobj *obj = sysobj_new_fast(PROC_MEMINFO);
        if (!obj->exists) {
            sysobj_free(obj);
            return NULL;
        }
        meminfo_path_fs = g_strdup(obj->path_fs);
        sysobj_read(obj, FALSE);
        gchar **lines = g_strsplit(obj->data.str, "\n", -1);
        for(int i = 0; lines[i]; i++) {
            gchar *c = strchr(lines[i], ':');
            if (c) {
                *c = 0;
                g_strchomp(lines[i]);
                sysobj_virt *vo = sysobj_virt_new();
                vo->path = g_strdup_printf(":/meminfo/%s", lines[i]);
                vo->type = VSO_TYPE_STRING;
                vo->f_get_data = meminfo_read;
                sysobj_virt_add(vo);
            }
        }
        g_strfreev(lines);

        sysobj_free(obj);
        return NULL;
    }

    return NULL; /* auto dir */
}

static gchar *meminfo_read(const gchar *path) {
    /* normal request */
    if (!meminfo_path_fs) return NULL;
    gchar *ret = NULL, *data = NULL;
    gchar *name = g_path_get_basename(path);
    if (name) g_strchomp(name);
    gg_file_get_contents_non_blocking(meminfo_path_fs, &data, NULL, NULL);
    if (data && name)
        ret = util_find_line_value(data, name, ':');
    g_free(data);
    g_free(name);
    return ret;
}

void gen_meminfo() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    /* to initialize */
    meminfo_scan("");
}

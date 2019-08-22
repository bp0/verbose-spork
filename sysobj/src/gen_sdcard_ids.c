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

/* Generator for ids from the sdcard.ids file
 *  :/lookup/sdcard.ids
 *
 * Items are generated on-demand and cached.
 *
 * :/lookup/sdcard.ids/MANID <id>/name
 * :/lookup/sdcard.ids/OEMID <id>/name
 *
 */
#include "sysobj.h"
#include "util_ids.h"

gboolean sdcard_ids_preload = TRUE;
static gchar *sdcard_ids_file = NULL;

#define sdcard_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

enum {
    PT_NONE = 0,
    PT_DIR  = 1,
    PT_NAME = 2,
};

static int _path_type(const gchar *path) {
    char name[5] = "";
    int mc = 0;
    unsigned int i1, i2;

    mc = sscanf(path, ":/lookup/sdcard.ids/MANFID %06x/%4s", &i1, name);
    if (mc == 2 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 1) return PT_DIR;

    mc = sscanf(path, ":/lookup/sdcard.ids/OEMID %04x/%4s", &i1, name);
    if (mc == 2 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 1) return PT_DIR;

    return PT_NONE;
}

int gen_sdcard_ids_lookup_type(const gchar *path) {
    if (SEQ(path, ":/lookup/sdcard.ids") )
        return VSO_TYPE_BASE;
    switch(_path_type(path)) {
        case PT_DIR: return VSO_TYPE_DIR;
        case PT_NAME: return VSO_TYPE_STRING;
    }
    return VSO_TYPE_NONE;
}

gchar *gen_sdcard_ids_lookup_value(const gchar *path) {
    if (!path) {
        /* cleanup */
        g_free(sdcard_ids_file);
        sdcard_ids_file = NULL;
        return NULL;
    }

    /* find the data file, if not already found */
    if (!sdcard_ids_file)
        sdcard_ids_file = sysobj_find_data_file("sdcard.ids");
    if (!sdcard_ids_file) {
        sdcard_msg("sdcard.ids file not found");
        return FALSE;
    }

    const gchar *qpath = path + strlen(":/lookup/sdcard.ids");
    if (!qpath)
        return NULL; /* auto-dir for lookup root */

    qpath++; /* skip the '/' */
    ids_query_result result = {};
    gchar *n = NULL;
    int pt = _path_type(path);
    if (!pt) return NULL; /* type will have been VSO_TYPE_NONE */

    scan_ids_file(sdcard_ids_file, qpath, &result, -1);

    gchar **qparts = g_strsplit(qpath, "/", -1);
    gchar *svo_path = g_strdup(":/lookup/sdcard.ids");
    for(int i = 0; qparts[i]; i++) {
        if (!SEQ(qparts[i], "name") ) {
            sysobj_virt_add_simple(svo_path, qparts[i], "*", VSO_TYPE_DIR);
            svo_path = appf(svo_path, "/", "%s", qparts[i]);
            n = result.results[i] ? result.results[i] : "";
            sysobj_virt_add_simple(svo_path, "name", n, VSO_TYPE_STRING);
        }
    }
    g_free(svo_path);
    g_strfreev(qparts);

    if (pt == PT_NAME) return g_strdup(n);
    return NULL; /* auto-dir */
}

static sysobj_virt vol[] = {
    { .path = ":/lookup/sdcard.ids", .str = "*",
      .type =  VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST | VSO_TYPE_CLEANUP,
      .f_get_data = gen_sdcard_ids_lookup_value, .f_get_type = gen_sdcard_ids_lookup_type },
};

gchar *split_loc_sdcard(const char *line) {
    gchar *fs = g_utf8_strchr(line, -1, ' ');
    if (fs)
        return g_utf8_strchr(fs+1, -1, ' ');
    else return NULL;
}

void gen_sdcard_ids() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    if (sdcard_ids_preload) {
        if (!sdcard_ids_file)
            sdcard_ids_file = sysobj_find_data_file("sdcard.ids");

        GSList *all = ids_file_all_get_all(sdcard_ids_file, split_loc_sdcard);
        GSList *l = all;
        for(; l; l = l->next) {
            ids_query *qr = l->data;
            gchar *n = NULL;
            gchar **qparts = g_strsplit(qr->qpath, "/", -1);
            gchar *svo_path = g_strdup(":/lookup/sdcard.ids");
            for(int i = 0; qparts[i]; i++) {
                sysobj_virt_add_simple(svo_path, qparts[i], "*", VSO_TYPE_DIR);
                svo_path = appf(svo_path, "/", "%s", qparts[i]);
                n = qr->result.results[i] ? qr->result.results[i] : "";
                sysobj_virt_add_simple(svo_path, "name", n, VSO_TYPE_STRING);
            }
            g_free(svo_path);
            g_strfreev(qparts);
        }
        g_slist_free_full(all, g_free);
    }
}

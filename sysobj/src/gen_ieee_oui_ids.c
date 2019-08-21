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

/* Generator for ids from the ieee_oui.ids file
 *  :/lookup/ieee_oui.ids
 *
 * Items are generated on-demand and cached.
 *
 * :/lookup/ieee_oui.ids/<3-letter vendor>/name
 *
 */
#include "sysobj.h"
#include "util_ids.h"

gboolean ieee_oui_ids_preload = FALSE;
static gchar *ieee_oui_ids_file = NULL;

#define ieee_oui_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

enum {
    PT_NONE = 0,
    PT_DIR  = 1,
    PT_NAME = 2,
};

static int _path_type(const gchar *path) {
    char name[5] = "";
    int mc = 0;
    unsigned int id;
    mc = sscanf(path, ":/lookup/ieee_oui.ids/%06x/%4s", &id, name);
    if (mc == 2 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 1) return PT_DIR;

    return PT_NONE;
}

int gen_ieee_oui_ids_lookup_type(const gchar *path) {
    if (SEQ(path, ":/lookup/ieee_oui.ids") )
        return VSO_TYPE_BASE;
    switch(_path_type(path)) {
        case PT_DIR: return VSO_TYPE_DIR;
        case PT_NAME: return VSO_TYPE_STRING;
    }
    return VSO_TYPE_NONE;
}

gchar *gen_ieee_oui_ids_lookup_value(const gchar *path) {
    if (!path) {
        /* cleanup */
        g_free(ieee_oui_ids_file);
        ieee_oui_ids_file = NULL;
        return NULL;
    }

    /* find the data file, if not already found */
    if (!ieee_oui_ids_file)
        ieee_oui_ids_file = sysobj_find_data_file("ieee_oui.ids");
    if (!ieee_oui_ids_file) {
        ieee_oui_msg("ieee_oui.ids file not found");
        return FALSE;
    }

    const gchar *qpath = path + strlen(":/lookup/ieee_oui.ids");
    if (!qpath)
        return NULL; /* auto-dir for lookup root */

    qpath++; /* skip the '/' */
    ids_query_result result = {};
    gchar *n = NULL;
    int pt = _path_type(path);
    if (!pt) return NULL; /* type will have been VSO_TYPE_NONE */

    scan_ids_file(ieee_oui_ids_file, qpath, &result, -1);

    gchar **qparts = g_strsplit(qpath, "/", -1);
    gchar *svo_path = g_strdup(":/lookup/ieee_oui.ids");
    for(int i = 0; qparts[i]; i++) {
        if (!SEQ(qparts[i], "name") ) {
            sysobj_virt_add_simple(svo_path, qparts[i], "*", VSO_TYPE_DIR);
            svo_path = appf(svo_path, "/", "%s", qparts[i]);
            n = result.results[i] ? result.results[i] : "";
            sysobj_virt_add_simple(svo_path, "name", n, VSO_TYPE_STRING);
        }
    }
    g_strfreev(qparts);

    if (pt == PT_NAME) return g_strdup(n);
    return NULL; /* auto-dir */
}

static sysobj_virt vol[] = {
    { .path = ":/lookup/ieee_oui.ids", .str = "*",
      .type =  VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST | VSO_TYPE_CLEANUP,
      .f_get_data = gen_ieee_oui_ids_lookup_value, .f_get_type = gen_ieee_oui_ids_lookup_type },
};

void gen_ieee_oui_ids() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    if (!ieee_oui_ids_file)
        ieee_oui_ids_file = sysobj_find_data_file("ieee_oui.ids");

    if (ieee_oui_ids_preload) {
        GSList *all = ids_file_all_get_all(ieee_oui_ids_file, NULL);
        GSList *l = all;
        for(; l; l = l->next) {
            ids_query *qr = l->data;
            gchar *n = NULL;
            gchar **qparts = g_strsplit(qr->qpath, "/", -1);
            gchar *svo_path = g_strdup(":/lookup/ieee_oui.ids");
            for(int i = 0; qparts[i]; i++) {
                sysobj_virt_add_simple(svo_path, qparts[i], "*", VSO_TYPE_DIR);
                svo_path = appf(svo_path, "/", "%s", qparts[i]);
                n = qr->result.results[i] ? qr->result.results[i] : "";
                sysobj_virt_add_simple(svo_path, "name", n, VSO_TYPE_STRING);
            }
            g_strfreev(qparts);
        }
        g_slist_free_full(all, g_free);
    }
}
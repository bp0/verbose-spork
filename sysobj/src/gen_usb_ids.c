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

/* Generator for ids from the usb.ids file (https://github.com/systemd/systemd/blob/master/hwdb/usb.ids)
 *  :/lookup/usb.ids
 *
 * Items are generated on-demand and cached.
 *
 * :/lookup/usb.ids/<vendor>/name
 * :/lookup/usb.ids/<vendor>/<device>/name
 * :/lookup/usb.ids/C <class>/name
 *
 */
#include "sysobj.h"
#include "util_ids.h"

static gchar *usb_ids_file = NULL;

#define usb_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

enum {
    PT_NONE = 0,
    PT_DIR  = 1,
    PT_NAME = 2,
};

static int _path_type(const gchar *path) {
    char name[5] = "";
    int mc = 0;
    unsigned int i1, i2, i3;

    mc = sscanf(path, ":/lookup/usb.ids/C %02x/%02x/%02x/%4s", &i1, &i2, &i3, name);
    if (mc == 4 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 3) return PT_DIR;

    mc = sscanf(path, ":/lookup/usb.ids/C %02x/%02x/%4s", &i1, &i2, name);
    if (mc == 3 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 2) return PT_DIR;

    mc = sscanf(path, ":/lookup/usb.ids/C %02x/%4s", &i1, name);
    if (mc == 2 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 1) return PT_DIR;

    mc = sscanf(path, ":/lookup/usb.ids/%04x/%04x/%4s", &i1, &i2, name);
    if (mc == 3 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 2) return PT_DIR;

    mc = sscanf(path, ":/lookup/usb.ids/%04x/%4s", &i1, name);
    if (mc == 2 && SEQ(name, "name") ) return PT_NAME;
    if (mc == 1) return PT_DIR;

    return PT_NONE;
}

int gen_usb_ids_lookup_type(const gchar *path) {
    if (SEQ(path, ":/lookup/usb.ids") )
        return VSO_TYPE_BASE;
    switch(_path_type(path)) {
        case PT_DIR: return VSO_TYPE_DIR;
        case PT_NAME: return VSO_TYPE_STRING;
    }
    return VSO_TYPE_NONE;
}

gchar *gen_usb_ids_lookup_value(const gchar *path) {
    if (!path) {
        /* cleanup */
        g_free(usb_ids_file);
        usb_ids_file = NULL;
        return NULL;
    }

    /* find the data file, if not already found */
    if (!usb_ids_file)
        usb_ids_file = sysobj_find_data_file("usb.ids");
    if (!usb_ids_file) {
        usb_msg("usb.ids file not found");
        return FALSE;
    }

    const gchar *qpath = path + strlen(":/lookup/usb.ids");
    if (!qpath)
        return NULL; /* auto-dir for lookup root */

    qpath++; /* skip the '/' */
    ids_query_result result = {};
    gchar *n = NULL;
    int pt = _path_type(path);
    if (!pt) return NULL; /* type will have been VSO_TYPE_NONE */

    scan_ids_file(usb_ids_file, qpath, &result, -1);

    gchar **qparts = g_strsplit(qpath, "/", -1);
    gchar *svo_path = g_strdup(":/lookup/usb.ids");
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
    { .path = ":/lookup/usb.ids", .str = "*",
      .type =  VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST | VSO_TYPE_CLEANUP,
      .f_get_data = gen_usb_ids_lookup_value, .f_get_type = gen_usb_ids_lookup_type },
};

void gen_usb_ids() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);
}

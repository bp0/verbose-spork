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

/* Generator for ids from the dt.ids file (https://github.com/bp0/dtid)
 *  :/devicetree/dt.ids
 *
 *
 * Items are generated on-demand and cached.
 *
 * :/devicetree/dt.ids/<compat_element>/vendor
 * :/devicetree/dt.ids/<compat_element>/name
 * :/devicetree/dt.ids/<compat_element>/class
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "sysobj.h"

#define ALLOW_WILDCARD 0

static const char dtids_file[] = "dt.ids";

#define dt_msg(...) fprintf (stderr, __VA_ARGS__)

typedef struct {
    gchar *compat_elem;
    gchar *match_str;
    gchar *name;
    gchar *vendor;
    gchar *class;
} dt_id;

#define dt_id_new() g_new0(dt_id, 1)

void dt_id_free(dt_id *id) {
    g_free(id->compat_elem);
    g_free(id->match_str);
    g_free(id->name);
    g_free(id->vendor);
    g_free(id->class);
    g_free(id);
}

static dt_id *scan_dtids_file(const char *compat_elem) {
#define DTID_BUFF_SIZE 128
    char buff[DTID_BUFF_SIZE];
    char last_vendor[DTID_BUFF_SIZE] = "";
    FILE *fd;
    char *cls, *cstr, *name;
    char *p, *b;
    int l;

    dt_id *ret = NULL;

    fd = fopen(dtids_file, "r");
    if (!fd) {
        dt_msg("dt.ids file could not be read\n");
        return NULL;
    }

    while (fgets(buff, DTID_BUFF_SIZE, fd)) {
        /* line ends at comment */
        p = strchr(buff, '#');
        if (p) *p = 0;

        /* trim trailing white space */
        p = buff + strlen(buff) - 1;
        while(p > buff && isspace((unsigned char)*p)) p--;
        *(p+1) = 0;

        /* scan for fields */
#define DTID_FFWD() while(isspace((unsigned char)*p)) p++;
        p = buff; DTID_FFWD();
        cls = p; if (!*cls) continue;
        b = strpbrk(p, " \t");
        if (!b) continue;
        *b = 0; p = b + 1; DTID_FFWD();
        cstr = p; if (!*cstr) continue;
        b = strpbrk(p, " \t");
        if (!b) continue;
        *b = 0; p = b + 1; DTID_FFWD();
        name = p; /* whatever is left is name */

        /* remember vendor */
        if (strcmp("vendor", cls) == 0)
            strcpy(last_vendor, name);

#if (ALLOW_WILDCARD)
        /* wildcard */
        b = strchr(cstr, '*');
        if (b) {
            *b = 0;
            l = strlen(cstr);
            if (strncmp(compat_elem, cstr, l) == 0)
                goto scan_dtid_done;
            else
                continue;
        }
#endif

        /* exact match */
        if (strcmp(compat_elem, cstr) == 0)
            goto scan_dtid_done;

    }
    name = NULL;

scan_dtid_done:
    fclose(fd);

    if (name) {
        ret = dt_id_new();
        /* convert to g_ */
        ret->compat_elem = g_strdup(compat_elem);
        ret->name = g_strdup(name);
        ret->vendor = strlen(last_vendor) ? g_strdup(last_vendor) : NULL;
        ret->class = g_strdup(cls);
        ret->match_str = g_strdup(cstr);
    }
    return ret;
}

static void gen_dt_ids_cache_item(dt_id *id) {
    gchar elpath[128] = "";

    if (!id) return;

    snprintf(elpath, 127, ":/devicetree/dt.ids/%s", id->compat_elem);
    sysobj_virt_add_simple(elpath, NULL, "*", VSO_TYPE_DIR);
    if (id->vendor)
        sysobj_virt_add_simple(elpath, "vendor", g_strdup(id->vendor), VSO_TYPE_STRING);
    if (id->name)
        sysobj_virt_add_simple(elpath, "name", g_strdup(id->name), VSO_TYPE_STRING);
    if (id->class)
        sysobj_virt_add_simple(elpath, "class", g_strdup(id->class), VSO_TYPE_STRING);
    if (id->match_str)
        sysobj_virt_add_simple(elpath, "match_str", g_strdup(id->match_str), VSO_TYPE_STRING);
}

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

static void buff_parent_name(const gchar *path, gchar *buff, gsize n) {
    gchar *ppath = g_path_get_dirname(path);
    gchar *pname = g_path_get_basename(ppath);
    strncpy(buff, pname, n);
    g_free(pname);
    g_free(ppath);
}

static gchar *gen_dt_ids_lookup_value(const gchar *path) {
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (!strcmp(name, "dt.ids") )
        return g_strdup("*");

    gchar *ret = NULL;

    char compat_elem[128] = "";
    sscanf(path, ":/devicetree/dt.ids/%s", compat_elem);
    if (compat_elem) {
        gchar *slash = strchr(compat_elem, '/');
        if (slash) *slash = 0;

        dt_id *id = scan_dtids_file(compat_elem);
        if (id) {
            gen_dt_ids_cache_item(id);
            if (!strcmp(name, "name") )
                ret = g_strdup(id->name);
            else if (!strcmp(name, "vendor") )
                ret = g_strdup(id->vendor);
            else if (!strcmp(name, "class") )
                ret = g_strdup(id->class);
            else if (!strcmp(name, "match_str") )
                ret = g_strdup(id->match_str);
            dt_id_free(id);
        } else {
            /* try for vendor, at least */
            gchar *comma = strchr(compat_elem, ',');
            if (comma) {
                *comma = 0;
                dt_id *idv = scan_dtids_file(compat_elem);
                if (idv) {
                    *comma = ',';
                    id = dt_id_new();
                    id->vendor = g_strdup(idv->name);
                    id->compat_elem = g_strdup(compat_elem);
                    dt_id_free(idv);

                    gen_dt_ids_cache_item(id);
                    if (!strcmp(name, "vendor") )
                        ret = g_strdup(id->vendor);

                    dt_id_free(id);
                }
            }
        }
    }

    return ret;
}

static int gen_dt_ids_lookup_type(const gchar *path) {
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (!strcmp(name, "dt.ids") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN;

    if (!strcmp(name, "name") )
        return VSO_TYPE_STRING;

    if (!strcmp(name, "vendor") )
        return VSO_TYPE_STRING;

    if (!strcmp(name, "class") )
        return VSO_TYPE_STRING;

    if (!strcmp(name, "match_str") )
        return VSO_TYPE_STRING;

    gchar pname[128] = "";
    buff_parent_name(path, pname, 127);
    /* assume compat_elem if child of dt.ids */
    if (!strcmp(pname, "dt.ids") )
        return VSO_TYPE_DIR;

    return VSO_TYPE_NONE;
}

void gen_dt_ids() {
    sysobj_virt *vo = sysobj_virt_new();
    vo->path = g_strdup(":/devicetree/dt.ids");
    vo->str = g_strdup("*");
    vo->type = VSO_TYPE_DIR | VSO_TYPE_DYN;
    vo->f_get_data = gen_dt_ids_lookup_value;
    vo->f_get_type = gen_dt_ids_lookup_type;
    sysobj_virt_add(vo);
}

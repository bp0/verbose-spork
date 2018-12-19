/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This file based on vendor.{h,c} from HardInfo
 * Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#define _GNU_SOURCE
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "sysobj.h"
#include "vendor.h"

static GSList *vendor_list = NULL;

/* sort the vendor list by length of match_string,
 * LONGEST first */
gint vendor_sort (gconstpointer a, gconstpointer b) {
    const Vendor *ap = a, *bp = b;
    int la = 0, lb = 0;
    if (ap && ap->match_string) la = strlen(ap->match_string);
    if (bp && bp->match_string) lb = strlen(bp->match_string);
    if (la == lb) return 0;
    if (la > lb) return -1;
    return 1;
}

static int read_from_vendor_ids(const char *path) {
#define VEN_BUFF_SIZE 128
#define VEN_FFWD() while(isspace((unsigned char)*p)) p++;
#define VEN_CHK(TOK) (strncmp(p, TOK, tl = strlen(TOK)) == 0)
    char buff[VEN_BUFF_SIZE] = "";
    char name[VEN_BUFF_SIZE] = "";
    char name_short[VEN_BUFF_SIZE] = "";
    char url[VEN_BUFF_SIZE] = "";
    char url_support[VEN_BUFF_SIZE] = "";
    char ansi_color[VEN_BUFF_SIZE] = "";
    int count = 0;
    FILE *fd;
    char *p, *b;
    int tl;

    DEBUG("using vendor.ids format loader for %s", path);

    fd = fopen(path, "r");
    if (!fd) return 0;

    while (fgets(buff, VEN_BUFF_SIZE, fd)) {
        b = strchr(buff, '\n');
        if (b) *b = 0;
        p = buff;
        VEN_FFWD();
        if (VEN_CHK("name ")) {
            strncpy(name, p + tl, VEN_BUFF_SIZE - 1);
            strcpy(name_short, "");
            strcpy(url, "");
            strcpy(url_support, "");
            strcpy(ansi_color, "");
        }
        if (VEN_CHK("name_short "))
            strncpy(name_short, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url "))
            strncpy(url, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url_support "))
            strncpy(url_support, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("ansi_color "))
            strncpy(ansi_color, p + tl, VEN_BUFF_SIZE - 1);

#define dup_if_not_empty(s) (strlen(s) ? g_strdup(s) : NULL)

        if (VEN_CHK("match_string ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->match_string = g_strdup(p+tl);
            v->match_case = 0;
            v->name = g_strdup(name);
            v->name_short = dup_if_not_empty(name_short);
            v->url = dup_if_not_empty(url);
            v->url_support = dup_if_not_empty(url_support);
            v->ansi_color = dup_if_not_empty(ansi_color);
            vendor_list = g_slist_prepend(vendor_list, v);
            count++;
        }

        if (VEN_CHK("match_string_case ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->match_string = g_strdup(p+tl);
            v->match_case = 1;
            v->name = g_strdup(name);
            v->name_short = dup_if_not_empty(name_short);
            v->url = dup_if_not_empty(url);
            v->url_support = dup_if_not_empty(url_support);
            v->ansi_color = dup_if_not_empty(ansi_color);
            vendor_list = g_slist_prepend(vendor_list, v);
            count++;
        }
    }

    fclose(fd);

    DEBUG("... found %d match strings", count);

    return count;
}

void vendor_init(void) {
    /* already initialized */
    if (vendor_list) return;

    DEBUG("initializing vendor list");

    gchar *path = sysobj_find_data_file("vendor.ids");
    int fail = 1;

    if (path && strstr(path, "vendor.ids")) {
        fail = !read_from_vendor_ids(path);
    }
    g_free(path);

    if (fail)
        DEBUG("vendor data not found");

    /* sort the vendor list by length of match string so that short strings are
     * less likely to incorrectly match.
     * example: ST matches ASUSTeK but SEAGATE is not ASUS */
    vendor_list = g_slist_sort(vendor_list, vendor_sort);
}

void vendor_cleanup() {
    DEBUG("cleanup vendor list");
    g_slist_free_full(vendor_list, (GDestroyNotify)vendor_free);
}

void vendor_free(Vendor *v) {
    if (v) {
        g_free(v->name);
        g_free(v->name_short);
        g_free(v->url);
        g_free(v->url_support);
        g_free(v->ansi_color);
        g_free(v->match_string);
        g_free(v);
    }
}

const Vendor *vendor_match(const gchar *id_str, ...) {
    va_list ap, ap2;
    GSList *vlp;
    gchar *tmp = NULL, *p = NULL;
    int tl = 0, c = 0;
    Vendor *ret = NULL;

    if (id_str) {
        c++;
        tl += strlen(id_str);
        tmp = g_malloc0(tl + c + 1);
        strcat(tmp, id_str);
        strcat(tmp, " ");

        va_start(ap, id_str);
        p = va_arg(ap, gchar*);
        while(p) {
            c++;
            tl += strlen(p);
            tmp = g_realloc(tmp, tl + c + 1); /* strings, spaces, null */
            strcat(tmp, p);
            strcat(tmp, " ");
            p = va_arg(ap, gchar*);
        }
        va_end(ap);
    }
    if (!c || tl == 0)
        return NULL;

    DEBUG("full id_str: %s", tmp);

    for (vlp = vendor_list; vlp; vlp = vlp->next) {
        Vendor *v = (Vendor *)vlp->data;

        if (v)
            if (v->match_case) {
                if (v->match_string && strstr(tmp, v->match_string)) {
                    ret = v;
                    break;
                }
            } else {
                if (v->match_string && strcasestr(tmp, v->match_string)) {
                    ret = v;
                    break;
                }
            }
    }

    if (ret)
        DEBUG("ret: match_string: %s -- case: %s -- name: %s", ret->match_string, (ret->match_case ? "yes" : "no"), ret->name);
    else
        DEBUG("ret: not found");

    g_free(tmp);
    return ret;
}

static const gchar *vendor_get_name_ex(const gchar * id_str, short shortest) {
    const Vendor *v = vendor_match(id_str, NULL);

    if (v) {
        if (shortest) {
            int sl = strlen(id_str);
            int nl = (v->name) ? strlen(v->name) : 0;
            int snl = (v->name_short) ? strlen(v->name_short) : 0;
            if (!nl) nl = 9999;
            if (!snl) snl = 9999;
            /* if id_str is shortest, then return as if
             *   not found (see below).
             * if all equal then prefer
             *   name_short > name > id_str. */
            if (nl < snl)
                return (sl < nl) ? id_str : v->name;
            else
                return (sl < snl) ? id_str : v->name_short;
        } else
            return v->name;
    }

    return id_str; /* Preserve an old behavior, but what about const? */
}

const gchar *vendor_get_name(const gchar * id_str) {
    return vendor_get_name_ex(id_str, 0);
}

const gchar *vendor_get_shortest_name(const gchar * id_str) {
    return vendor_get_name_ex(id_str, 1);
}

const gchar *vendor_get_url(const gchar * id_str) {
    const Vendor *v = vendor_match(id_str, NULL);

    if (v)
        return v->url;

    return NULL;
}

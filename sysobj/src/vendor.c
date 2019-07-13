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
#include "strstr_word.h"

#define ven_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/
#define ven_msg_debug(msg, ...)  /* fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) */

static vendor_list vendors = NULL;
const vendor_list get_vendors_list() { return vendors; }

/* sort the vendor list by length of match_string,
 * LONGEST first */
int vendor_sort (const Vendor *ap, const Vendor *bp) {
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
#define VEN_CHK(TOK) (strncmp(p, TOK, tl = strlen(TOK)) == 0 && (ok = 1))
    char buff[VEN_BUFF_SIZE] = "";

    char vars[7][VEN_BUFF_SIZE];
    char *name = vars[0];
    char *name_short = vars[1];
    char *url = vars[2];
    char *url_support = vars[3];
    char *wikipedia = vars[4];
    char *note = vars[5];
    char *ansi_color = vars[6];

    int count = 0;
    FILE *fd;
    char *p, *b;
    int tl, line = -1, ok = 0;

    ven_msg_debug("using vendor.ids format loader for %s", path);

    fd = fopen(path, "r");
    if (!fd) return 0;

    while (fgets(buff, VEN_BUFF_SIZE, fd)) {
        ok = 0;
        line++;

        b = strchr(buff, '\n');
        if (b)
            *b = 0;
        else
            ven_msg("%s:%d: line longer than VEN_BUFF_SIZE (%lu)", path, line, (unsigned long)VEN_BUFF_SIZE);

        b = strchr(buff, '#');
        if (b) *b = 0; /* line ends at comment */

        p = buff;
        VEN_FFWD();
        if (VEN_CHK("name ")) {
            strncpy(name, p + tl, VEN_BUFF_SIZE - 1);
            strcpy(name_short, "");
            strcpy(url, "");
            strcpy(url_support, "");
            strcpy(wikipedia, "");
            strcpy(note, "");
            strcpy(ansi_color, "");
        }
        if (VEN_CHK("name_short "))
            strncpy(name_short, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url "))
            strncpy(url, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("url_support "))
            strncpy(url_support, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("wikipedia "))
            strncpy(wikipedia, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("note "))
            strncpy(note, p + tl, VEN_BUFF_SIZE - 1);
        if (VEN_CHK("ansi_color "))
            strncpy(ansi_color, p + tl, VEN_BUFF_SIZE - 1);

#define dup_if_not_empty(s) (strlen(s) ? g_strdup(s) : NULL)

        if (VEN_CHK("match_string ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->file_line = line;
            v->match_string = g_strdup(p+tl);
            v->ms_length = strlen(v->match_string);
            v->match_rule = 0;
            v->name = g_strdup(name);
            v->name_short = dup_if_not_empty(name_short);
            v->url = dup_if_not_empty(url);
            v->url_support = dup_if_not_empty(url_support);
            v->wikipedia = dup_if_not_empty(wikipedia);
            v->note = dup_if_not_empty(note);
            v->ansi_color = dup_if_not_empty(ansi_color);
            vendors = g_slist_prepend(vendors, v);
            count++;
        }

        if (VEN_CHK("match_string_case ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->file_line = line;
            v->match_string = g_strdup(p+tl);
            v->ms_length = strlen(v->match_string);
            v->match_rule = 1;
            v->name = g_strdup(name);
            v->name_short = dup_if_not_empty(name_short);
            v->url = dup_if_not_empty(url);
            v->url_support = dup_if_not_empty(url_support);
            v->wikipedia = dup_if_not_empty(wikipedia);
            v->note = dup_if_not_empty(note);
            v->ansi_color = dup_if_not_empty(ansi_color);
            vendors = g_slist_prepend(vendors, v);
            count++;
        }

        if (VEN_CHK("match_string_exact ")) {
            Vendor *v = g_new0(Vendor, 1);
            v->file_line = line;
            v->match_string = g_strdup(p+tl);
            v->ms_length = strlen(v->match_string);
            v->match_rule = 2;
            v->name = g_strdup(name);
            v->name_short = dup_if_not_empty(name_short);
            v->url = dup_if_not_empty(url);
            v->url_support = dup_if_not_empty(url_support);
            v->wikipedia = dup_if_not_empty(wikipedia);
            v->note = dup_if_not_empty(note);
            v->ansi_color = dup_if_not_empty(ansi_color);
            vendors = g_slist_prepend(vendors, v);
            count++;
        }

        g_strstrip(buff);
        if (!ok && *buff != 0)
            ven_msg("unrecognised item at %s:%d, %s", path, line, p);
    }

    fclose(fd);

    ven_msg_debug("... found %d match strings", count);

    return count;
}

void vendor_init(void) {
    /* already initialized */
    if (vendors) return;

    ven_msg_debug("initializing vendor list");

    gchar *path = sysobj_find_data_file("vendor.ids");
    int fail = 1;

    if (path && strstr(path, "vendor.ids")) {
        fail = !read_from_vendor_ids(path);
    }
    g_free(path);

    if (fail)
        ven_msg_debug("vendor data not found");

    /* sort the vendor list by length of match string so that short strings are
     * less likely to incorrectly match.
     * example: ST matches ASUSTeK but SEAGATE is not ASUS */
    vendors = g_slist_sort(vendors, (GCompareFunc)vendor_sort);
}

void vendor_cleanup() {
    ven_msg_debug("cleanup vendor list");
    g_slist_free_full(vendors, (GDestroyNotify)vendor_free);
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
    Vendor *ret = NULL;
    va_list ap, ap2;
    gchar *tmp = NULL, *p = NULL;
    int tl = 0, c = 0;

    if (id_str) {
        c++;
        tl += strlen(id_str);
        tmp = appf(tmp, "%s", id_str);

        va_start(ap, id_str);
        p = va_arg(ap, gchar*);
        while(p) {
            c++;
            tl += strlen(p);
            tmp = appf(tmp, "%s", p);
            p = va_arg(ap, gchar*);
        }
        va_end(ap);
    }
    if (!c || tl == 0)
        return NULL;

    vendor_list vl = vendors_match_core(tmp, 1);
    if (vl) {
        ret = (Vendor*)vl->data;
        vendor_list_free(vl);
    }
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

vendor_list vendor_list_concat_va(int count, vendor_list vl, ...) {
    vendor_list ret = vl, p = NULL;
    va_list ap;
    va_start(ap, vl);
    if (count > 0) {
        count--; /* includes vl */
        while (count) {
            p = va_arg(ap, vendor_list);
            ret = g_slist_concat(ret, p);
            count--;
        }
    } else {
        p = va_arg(ap, vendor_list);
        while (p) {
            ret = g_slist_concat(ret, p);
            p = va_arg(ap, vendor_list);
        }
    }
    va_end(ap);
    return ret;
}

vendor_list vendor_list_remove_duplicates_deep(vendor_list vl) {
    /* vendor_list is GSList* */
    GSList *tvl = vl;
    GSList *evl = NULL;
    while(tvl) {
        const Vendor *tv = tvl->data;
        evl = tvl->next;
        while(evl) {
            const Vendor *ev = evl->data;
            if ( SEQ(ev->name, tv->name)
                 && SEQ(ev->name_short, tv->name_short)
                 && SEQ(ev->ansi_color, tv->ansi_color)
                 && SEQ(ev->url, tv->url)
                 && SEQ(ev->url_support, tv->url_support)
                 && SEQ(ev->wikipedia, tv->wikipedia)
                 ) {
                GSList *next = evl->next;
                vl = g_slist_delete_link(vl, evl);
                evl = next;
            } else
                evl = evl->next;
        }
        tvl = tvl->next;
    }
    return vl;
}

vendor_list vendors_match(const gchar *id_str, ...) {
    va_list ap, ap2;
    gchar *tmp = NULL, *p = NULL;
    int tl = 0, c = 0;

    if (id_str) {
        c++;
        tl += strlen(id_str);
        tmp = appf(tmp, "%s", id_str);

        va_start(ap, id_str);
        p = va_arg(ap, gchar*);
        while(p) {
            c++;
            tl += strlen(p);
            tmp = appf(tmp, "%s", p);
            p = va_arg(ap, gchar*);
        }
        va_end(ap);
    }
    if (!c || tl == 0)
        return NULL;

    return vendors_match_core(tmp, -1);
}

vendor_list vendors_match_core(const gchar *str, int limit) {
    gchar *p = NULL;
    GSList *vlp;
    int found = 0;
    vendor_list ret = NULL;

    /* first pass (passes[1]): ignore text in (),
     *     like (formerly ...) or (nee ...)
     * second pass (passes[0]): full text */
    gchar *passes[2] = { g_strdup(str), g_strdup(str) };
    int pass = 1; p = passes[1];
    while(p = strchr(p, '(') ) {
        pass = 2; p++;
        while(*p && *p != ')') { *p = ' '; p++; }
    }

    for (; pass > 0; pass--) {
        for (vlp = vendors; vlp; vlp = vlp->next) {
            sysobj_stats.ven_iter++;
            Vendor *v = (Vendor *)vlp->data;
            char *m = NULL;

            if (!v) continue;
            if (!v->match_string) continue;

            switch(v->match_rule) {
                case 0:
                    if (m = strcasestr_word(passes[pass-1], v->match_string) ) {
                        /* clear so it doesn't match again */
                        for(char *s = m; s < m + v->ms_length; s++) *s = ' ';
                        /* add to return list */
                        ret = vendor_list_append(ret, v);
                        found++;
                        if (limit > 0 && found >= limit)
                            goto vendors_match_core_finish;
                    }
                    break;
                case 1: /* match case */
                    if (m = strstr_word(passes[pass-1], v->match_string) ) {
                        /* clear so it doesn't match again */
                        for(char *s = m; s < m + v->ms_length; s++) *s = ' ';
                        /* add to return list */
                        ret = vendor_list_append(ret, v);
                        found++;
                        if (limit > 0 && found >= limit)
                            goto vendors_match_core_finish;
                    }
                    break;
                case 2: /* match exact */
                    if (SEQ(passes[pass-1], v->match_string) ) {
                        ret = vendor_list_append(ret, v);
                        found++;
                        goto vendors_match_core_finish; /* no way for any other match to happen */
                    }
                    break;
            }
        }
    }

vendors_match_core_finish:

    g_free(passes[0]);
    g_free(passes[1]);
    return ret;
}

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

#include "util_ids.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define ids_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

ids_query *ids_query_new(gchar *qpath) {
    ids_query *s = g_new0(ids_query, 1);
    s->qpath = qpath;
    return s;
}

void ids_query_free(ids_query *s) {
    if (s) g_free(s->qpath);
    g_free(s);
}

/* Given a qpath "/X/Y/Z", find names as:
 * X <name> ->result[0]
 * \tY <name> ->result[1]
 * \t\tZ <name> ->result[2]
 *
 * Works with:
 * - pci.ids "<vendor>/<device>/<subvendor> <subdevice>" or "C <class>/<subclass>/<prog-if>"
 * - arm.ids "<implementer>/<part>"
 * - sdio.ids "<vendor>/<device>", "C <class>"
 * - usb.ids "<vendor>/<device>", "C <class>" etc, but file_unsorted = TRUE
 *   because usb.ids has several prefixed sets that are not in sorted order.
 *   What happened to "Please keep sorted"?!
 */
long scan_ids_file(const gchar *file, const gchar *qpath, gboolean file_unsorted, ids_query_result *result, long start_offset) {
    gchar **qparts = NULL;
    gchar buff[IDS_LOOKUP_BUFF_SIZE] = "";
    ids_query_result ret = {};
    gchar *p = NULL;

    FILE *fd;
    int tabs;
    int qdepth;
    int qpartlen[IDS_LOOKUP_MAX_DEPTH];
    long last_root_fpos = -1, fpos;

    if (!qpath)
        return -1;

    fd = fopen(file, "r");
    if (!fd) {
        ids_msg("file could not be read: %s", file);
        return -1;
    }

    qparts = g_strsplit(qpath, "/", -1);
    qdepth = g_strv_length(qparts);
    if (qdepth > IDS_LOOKUP_MAX_DEPTH) {
        ids_msg("qdepth (%d) > ids_max_depth (%d) for %s", qdepth, IDS_LOOKUP_MAX_DEPTH, qpath);
        qdepth = IDS_LOOKUP_MAX_DEPTH;
    }
    for(int i = 0; i < qdepth; i++)
        qpartlen[i] = strlen(qparts[i]);

    if (start_offset > 0)
        fseek(fd, start_offset, SEEK_SET);

    for (fpos = ftell(fd); fgets(buff, IDS_LOOKUP_BUFF_SIZE, fd); fpos = ftell(fd)) {
        /* line ends at comment */
        p = strchr(buff, '#');
        if (p) *p = 0;
        /* trim trailing white space */
        if (!p) p = buff + strlen(buff);
        p--;
        while(p > buff && isspace((unsigned char)*p)) p--;
        *(p+1) = 0;
        p = buff;

        if (buff[0] == 0)    continue; /* empty line */
        if (buff[0] == '\n') continue; /* empty line */

        /* scan for fields */
        tabs = 0;
        while(*p == '\t') { tabs++; p++; }

        if (tabs >= qdepth) continue; /* too deep */
        if (tabs != 0 && !ret.results[tabs-1])
            continue; /* not looking at this depth, yet */

        if (g_str_has_prefix(p, qparts[tabs])
            && isspace(*(p + qpartlen[tabs])) ) {
            /* found */
            p += qpartlen[tabs];
            while(isspace((unsigned char)*p)) p++; /* ffwd */

            if (tabs == 0) {
                last_root_fpos = fpos;
                ret.results[tabs] = ret._strs;
                strncpy(ret.results[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
            } else {
                ret.results[tabs] = ret.results[tabs-1] + strlen(ret.results[tabs-1]) + 1;
                strncpy(ret.results[tabs], p, IDS_LOOKUP_BUFF_SIZE-1);
            }
            continue;
        }

        if (!file_unsorted && g_strcmp0(p, qparts[tabs]) == 1)
            goto ids_lookup_done; /* will not be found */

    } /* for each line */

ids_lookup_done:
    fclose(fd);

    if (result) {
        memcpy(result, &ret, sizeof(ids_query_result));
        for(int i = 0; result->results[i]; i++)
            result->results[i] = result->_strs + (ret.results[i] - ret._strs);

        return last_root_fpos;
    }
    return last_root_fpos;
}

static gint _ids_query_list_cmp(const ids_query *ql1, const ids_query *ql2) {
    return g_strcmp0(ql1->qpath, ql2->qpath);
}

long scan_ids_file_list(const gchar *file, gboolean file_unsorted, GSList *query_list, long start_offset) {
    GSList *tmp = g_slist_copy(query_list);
    tmp = g_slist_sort(tmp, (GCompareFunc)_ids_query_list_cmp);

    long offset = start_offset;
    for (GSList *l = query_list; l; l = l->next) {
        ids_query *q = l->data;
        offset = scan_ids_file(file, q->qpath, file_unsorted, &(q->result), offset);
        if (offset == -1 && !file_unsorted)
            break;
    }
    g_slist_free(tmp);
    return offset;
}

long query_list_cound_found(GSList *query_list) {
    long count = 0;
    for (GSList *l = query_list; l; l = l->next) {
        ids_query *q = l->data;
        if (q->result.results[0]) count++;
    }
    return count;
}

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
#include "sysobj_filter.h"

sysobj_filter *sysobj_filter_new(int type, const gchar *pattern) {
    sysobj_filter *f = g_new0(sysobj_filter, 1);
    f->pattern = g_strdup(pattern);
    f->type = type;
    f->pspec = g_pattern_spec_new(f->pattern);
    return f;
}

void sysobj_filter_free(sysobj_filter *f) {
    if (f) {
        if (f->pspec)
            g_pattern_spec_free(f->pspec);

        if (!(f->type & SO_FILTER_STATIC) ) {
            g_free(f->pattern);
            g_free(f);
        }
    }
}

gboolean sysobj_filter_item_include(gchar *item, GSList *filters) {
    if (!item) return FALSE;

    sysobj_filter *f = NULL;
    gboolean marked = FALSE;
    GSList *fp = filters;
    gsize len = strlen(item);

    while (fp) {
        f = fp->data;
        if (!f->pspec)
            f->pspec = g_pattern_spec_new(f->pattern);
        gboolean match = g_pattern_match(f->pspec, len, item, NULL);
        switch (f->type & SO_FILTER_MASK) {
            case SO_FILTER_EXCLUDE_IIF:
                marked = match;
                break;
            case SO_FILTER_INCLUDE_IIF:
                marked = !match;
                break;
            case SO_FILTER_EXCLUDE:
                if (match) marked = TRUE;
                break;
            case SO_FILTER_INCLUDE:
                if (match) marked = FALSE;
                break;
        }

        /*
        DEBUG(" pattern: %s %s%s -> %s -> %s", f->pattern,
            f->type & SO_FILTER_EXCLUDE ? "ex" : "inc", f->type & SO_FILTER_IIF ? "_iif" : "",
            match ? "match" : "no-match", marked ? "marked" : "not-marked"); */

        fp = fp->next;
    }

    return !marked;
}

GSList *sysobj_filter_list(GSList *items, GSList *filters) {
    GSList *fp = filters, *l = NULL, *n = NULL;
    sysobj_filter *f = NULL;
    gboolean *marked;
    gsize i = 0, num_items = g_slist_length(items);

    marked = g_new(gboolean, num_items);
    while (fp) {
        f = fp->data;
        if (!f->pspec)
            f->pspec = g_pattern_spec_new(f->pattern);
        l = items;
        i = 0;
        while (l) {
            gchar *d = l->data;
            gsize len = strlen(d);
            gboolean match = g_pattern_match(f->pspec, len, d, NULL);
            switch (f->type) {
                case SO_FILTER_EXCLUDE_IIF:
                    marked[i] = match;
                    break;
                case SO_FILTER_INCLUDE_IIF:
                    marked[i] = !match;
                    break;
                case SO_FILTER_EXCLUDE:
                    if (match) marked[i] = TRUE;
                    break;
                case SO_FILTER_INCLUDE:
                    if (match) marked[i] = FALSE;
                    break;
            }
            l = l->next;
            i++;
        }
        fp = fp->next;
    }

    l = items;
    i = 0;
    while (l) {
        n = l->next;
        if (marked[i])
            items = g_slist_delete_link(items, l);
        l = n;
        i++;
    }

    g_free(marked);
    return items;
}

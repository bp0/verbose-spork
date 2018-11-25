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

#include "sysobj_foreach.h"

static GList* __push_if_uniq(GList *l, gchar *base, gchar *name) {
    gchar *path = name
        ? g_strdup_printf("%s/%s", base, name)
        : g_strdup(base);
    GList *a = NULL;
    for(a = l; a; a = a->next) {
        if (!g_strcmp0((gchar*)a->data, path) ) {
            g_free(path);
            return l;
        }
    }
    return g_list_append(l, path);
}

static GList* __shift(GList *l, gchar **s) {
    *s = (gchar*)l->data;
    return g_list_delete_link(l, l);
}

static gboolean _has_symlink_loop(const gchar *path) {
    GList *targets = NULL;
    sysobj *obj = sysobj_new_from_fn(path, NULL);
    sysobj *next = NULL;
    while(obj) {
        if ( g_list_find_custom(targets, obj->path, (GCompareFunc)g_strcmp0) )
            goto loop_check_fail;

        targets = g_list_prepend(targets, g_strdup(obj->path) );

        /* request parent */
        next = sysobj_parent(obj);
        sysobj_free(obj);
        obj = next;
    }
    g_list_free_full(targets, g_free);
    return FALSE;

loop_check_fail:
    sysobj_free(obj);
    g_list_free_full(targets, g_free);
    return TRUE;
}

void sysobj_foreach(GSList *filters, f_sysobj_foreach callback, gpointer user_data) {
    guint searched = 0;
    GList *to_search = NULL;
    to_search = __push_if_uniq(to_search, ":/", NULL);
    gchar *path = NULL;
    while( g_list_length(to_search) ) {
        to_search = __shift(to_search, &path);
        printf("to_search:%d searched:%d now: %s\n", g_list_length(to_search), searched, path );
        if (_has_symlink_loop(path) ) { g_free(path); continue; }

        sysobj *obj = sysobj_new_from_fn(path, NULL);
        if (!obj) { g_free(path); continue; }
        if (filters
            && !sysobj_filter_item_include(obj->path, filters) ) {
                sysobj_free(obj);
                g_free(path);
                continue;
            }

        /* callback */
        if ( callback(obj, user_data) == SYSOBJ_FOREACH_STOP ) {
            sysobj_free(obj);
            g_free(path);
            break;
        }
        searched++;

        /* scan children */
        if (obj->data.is_dir) {
            sysobj_read(obj, FALSE);
            const GSList *lc = obj->data.childs;
            for (lc = obj->data.childs; lc; lc = lc->next) {
                to_search = __push_if_uniq(to_search, obj->path_req, (gchar*)lc->data);
            }
        }

        sysobj_free(obj);
        g_free(path);
    }
    g_list_free_full(to_search, g_free);
}

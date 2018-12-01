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

#include "sysobj_collection.h"

static void _value_destroy_func(sysobj *s) {
    sysobj_free(s);
}

sysobj_collection *sysobj_collection_new() {
    sysobj_collection *s = g_new0(sysobj_collection, 1);
    s->items = g_hash_table_new_full((GHashFunc)g_str_hash, (GEqualFunc)g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)_value_destroy_func);
    return s;
}

sysobj_collection *sysobj_collection_children_of(sysobj *obj) {
    GSList *l = NULL;
    sysobj_collection *s = sysobj_collection_new();
    if (obj->data.is_dir) {
        sysobj_read(obj, FALSE);
        for (l = obj->data.childs; l; l = l->next) {
            sysobj *new_obj = sysobj_new_from_fn(obj->path_req, l->data);
            g_hash_table_replace(s->items, g_strdup(new_obj->path_req), new_obj);
        }
    }
    return s;
}

void sysobj_collection_free(sysobj_collection *s) {
    if (s) {
        g_hash_table_destroy(s->items);
        g_free(s);
    }
}

sysobj *sysobj_collection_get(sysobj_collection *s, const gchar *base, const gchar *name) {
    gchar *path = util_build_fn(base, name);
    sysobj *ret = g_hash_table_lookup(s->items, path);
    g_free(path);
    return ret;
}

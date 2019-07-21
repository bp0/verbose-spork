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

#include "sysobj.h"

static vendor_list storage_all_vendors(sysobj *obj) {
    GSList *ret = NULL;
    if (SEQ(":/storage", obj->path)) {
        GSList *childs = sysobj_children(obj, "storage*", NULL, TRUE);
        for(GSList *l = childs; l; l = l->next)
            ret = vendor_list_concat(ret, sysobj_vendors_from_fn(obj->path, l->data));
        g_slist_free_full(childs, g_free);
    }
    return ret;
}

static gchar *storage_list_format(sysobj *obj, int fmt_opts) {
    if (SEQ(":/storage", obj->path)) {
        gchar *ret = NULL;
        GSList *childs = sysobj_children(obj, "storage*", NULL, TRUE);
        for(GSList *l = childs; l; l = l->next) {
            gchar *storage = sysobj_format_from_fn(obj->path, l->data, fmt_opts);
            if (storage)
                ret = appf(ret, " + ", "%s", storage);
            g_free(storage);
        }
        g_slist_free_full(childs, g_free);
        if (ret)
            return ret;
    }
    return simple_format(obj, fmt_opts);
}

static sysobj_class cls_storage[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "storage_list", .pattern = ":/storage", .flags = OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("list of discovered storage devices"),
    .f_format = storage_list_format, .f_vendors = storage_all_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "storage_item", .pattern = ":/storage/storage*", .flags = OF_CONST | OF_HAS_VENDOR | OF_GLOB_PATTERN,
    .v_lblnum = "storage", .s_node_format = "{{device}}", .s_vendors_from_child = "device" }
};

void class_storage() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_storage); i++)
        class_add(&cls_storage[i]);
}

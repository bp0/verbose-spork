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

/* Generator for storage information tree
 *  :/storage
 */
#include "sysobj.h"

#define storage_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

static int storage_next = 0;

typedef struct {
    int type;
    gchar *name; /* storage<storage_next> */
} storaged;

#define storaged_new() (g_new0(storaged, 1))
static void storaged_free(storaged *s) {
    if (!s) return;
    g_free(s->name);
    g_free(s);
}

static GSList *storage_list = NULL;
#define storage_list_free() g_slist_free_full(storage_list, (GDestroyNotify)storaged_free)

static GMutex storage_list_lock;

void storage_item(sysobj *obj) {
    gchar *name = g_strdup_printf("storage%d", storage_next++);
    gchar *dev = g_strdup_printf("%s/device", name);
    sysobj_virt_add_simple(":/storage", name, "*", VSO_TYPE_DIR);
    sysobj_virt_add_simple(":/storage", dev, obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN);
    g_free(name);
    g_free(dev);
}

void storage_scan() {
    sysobj *blist = sysobj_new_fast("sys/block");
    GSList *childs = sysobj_children(blist, NULL, NULL, TRUE);
    for(GSList *l = childs; l; l = l->next) {
        gchar *devpath = g_strdup_printf("%s/%s/device", blist->path, (gchar*)l->data);
        sysobj *bdev = sysobj_new_fast(devpath);
        if (sysobj_exists(bdev))
            storage_item(bdev);
        sysobj_free(bdev);
    }
    g_slist_free_full(childs, (GDestroyNotify)g_free);
    sysobj_free(blist);
}

int storage_get_type(const gchar *path) {
    if (SEQ(path, ":/storage") )
        return VSO_TYPE_BASE;

    return VSO_TYPE_NONE;
}

gchar *storage_get_data(const gchar *path) {
    if (!path) return NULL;

    if (SEQ(path, ":/storage") )
        return "*";

    return NULL;
}

static sysobj_virt vol[] = {
    { .path = ":/storage", .str = "*",
      .f_get_type = storage_get_type,
      .f_get_data = storage_get_data,
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
};

void gen_storage() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    storage_scan();
}

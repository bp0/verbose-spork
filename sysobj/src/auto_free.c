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

#include "auto_free.h"
#include "sysobj.h"

static GMutex free_lock;
static GSList *free_list = NULL;
static gboolean free_final = FALSE;

typedef struct {
    gpointer ptr;
    GThread *thread;
    GDestroyNotify f_free;
    double stamp;

    const char *file;
    int  line;
    const char *func;
} auto_free_item;

gpointer auto_free_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func) {
    if (!p) return p;

    auto_free_item *z = g_new0(auto_free_item, 1);
    z->ptr = p;
    z->f_free = f;
    z->thread = g_thread_self();
    z->file = file;
    z->line = line;
    z->func = func;
    z->stamp = sysobj_elapsed();
    g_mutex_lock(&free_lock);
    free_list = g_slist_prepend(free_list, z);
    sysobj_stats.auto_free_len++;
    g_mutex_unlock(&free_lock);
    return p;
}

gpointer auto_free_(gpointer p, const char *file, int line, const char *func) {
    return auto_free_ex_(p, (GDestroyNotify)g_free, file, line, func);
}

static struct { GDestroyNotify fptr; char *name; }
    free_function_tab[] = {
    { (GDestroyNotify) g_free,             "g_free" },
    { (GDestroyNotify) sysobj_free,        "sysobj_free" },
    { (GDestroyNotify) class_free,         "class_free" },
    { (GDestroyNotify) sysobj_filter_free, "sysobj_filter_free" },
    { (GDestroyNotify) sysobj_virt_free,   "sysobj_virt_free" },
    { NULL, "(null)" },
};

void free_auto_free_final() {
    free_final = TRUE;
    free_auto_free();
}

void free_auto_free() {
    GThread *this_thread = g_thread_self();
    GSList *l = NULL, *n = NULL;
    long long unsigned fc = 0;
    double now = sysobj_elapsed();

    if (!free_list) return;

    g_mutex_lock(&free_lock);
    DEBUG("%llu total items in queue, but will free from thread %p only... ", sysobj_stats.auto_free_len, this_thread);
    for(l = free_list; l; l = n) {
        auto_free_item *z = (auto_free_item*)l->data;
        n = l->next;
        double age = now - z->stamp;
        if (free_final || (z->thread == this_thread && age > AF_DELAY_SECONDS) ) {
            if (DEBUG_AUTO_FREE) {
                char fptr[128] = "", *fname;
                for(int i = 0; i < (int)G_N_ELEMENTS(free_function_tab); i++)
                    if (z->f_free == free_function_tab[i].fptr)
                        fname = free_function_tab[i].name;
                if (!fname) {
                    snprintf(fname, 127, "%p", z->f_free);
                    fname = fptr;
                }
                if (z->file || z->func)
                    DEBUG("free: %s(%p) age:%lfs from %s:%d %s()", fname, z->ptr, age, z->file, z->line, z->func);
                else
                    DEBUG("free: %s(%p) age:%lfs", fname, z->ptr, age);
            }

            z->f_free(z->ptr);
            g_free(z);
            free_list = g_slist_delete_link(free_list, l);
            fc++;
        }
    }
    DEBUG("... freed %llu (from thread %p)", fc, this_thread);
    sysobj_stats.auto_freed += fc;
    sysobj_stats.auto_free_len -= fc;
    g_mutex_unlock(&free_lock);
}

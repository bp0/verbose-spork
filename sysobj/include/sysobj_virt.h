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

#ifndef _SYSOBJ_VIRT_H_
#define _SYSOBJ_VIRT_H_

#include "sysobj.h"

enum {
    VSO_TYPE_NONE     = 0,    /* if returned from f_get_type(), signal not found */
    VSO_TYPE_DIR      = 1,
    VSO_TYPE_STRING   = 1<<1,

    VSO_TYPE_REQ_ROOT = 1<<15,
    VSO_TYPE_SYMLINK  = 1<<16,
    VSO_TYPE_DYN      = 1<<17, /* any path beyond that doesn't match a non-dyn path */
    VSO_TYPE_AUTOLINK = 1<<18,
    VSO_TYPE_CLEANUP  = 1<<19, /* will get a call f_get_data(NULL) to signal cleanup */
    VSO_TYPE_BASE     = 1<<20, /* for DYN vo's, return from f_get_type() with the the base type */
    VSO_TYPE_CONST    = 1<<30, /* don't free */
};

typedef struct sysobj_virt {
    gchar *path;
    int type;
    gchar *str; /* default f_get_data() uses str; */
    gchar *(*f_get_data)(const gchar *path); /* f_get_data(NULL) is called for cleanup */
    int (*f_get_type)(const gchar *path);
} sysobj_virt;

#define sysobj_virt_new() g_new0(sysobj_virt, 1)
void sysobj_virt_free(sysobj_virt *s);
gboolean sysobj_virt_add(sysobj_virt *vo); /* TRUE if added, FALSE if exists (was overwritten) or error */
gboolean sysobj_virt_add_simple(const gchar *base, const gchar *name, const gchar *data, int type);
gboolean sysobj_virt_add_simple_mkpath(const gchar *base, const gchar *name, const gchar *data, int type);
void sysobj_virt_remove(gchar *glob);
sysobj_virt *sysobj_virt_find(const gchar *path);
gchar *sysobj_virt_get_data(const sysobj_virt *vo, const gchar *req);
int sysobj_virt_get_type(const sysobj_virt *vo, const gchar *req);
GSList *sysobj_virt_all_paths();
int sysobj_virt_count();
/* using the glib key-value file parser, create a tree of
 * base/group/name=value virtual sysobj's. Items before a first
 * group are put in base/name=value. */
void sysobj_virt_from_kv(const gchar *base, const gchar *kv_data_in);
/* lines in the form "key: value" */
void sysobj_virt_from_lines(const gchar *base, const gchar *data_in, gboolean safe_names);

gchar *sysobj_virt_is_symlink(gchar *req); /* returns the link or null if not a link */
GSList *sysobj_virt_children(const sysobj_virt *vo, const gchar *req);
void sysobj_virt_cleanup();

#endif

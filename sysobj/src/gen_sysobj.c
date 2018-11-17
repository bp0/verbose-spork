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

static gchar *get_root(const gchar *path) {
    return g_strdup_printf("%s", sysobj_root_get());
}

static gchar *get_elapsed(const gchar *path) {
    return g_strdup_printf("%lf", sysobj_elapsed());
}

static sysobj_virt vol[] = {
    /* the vsysfs  root */
    { .path = ":", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },

    /* handy /sys and /proc links */
    { .path = ":/sysfs", .str = "/sys",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":/procfs", .str = "/proc",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },

    /* for the sysobj_watchlist*() functions */
    { .path = ":/watchlist", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },

    /* internal stuff */
    { .path = ":sysobj", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":sysobj/elapsed", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = get_elapsed, .f_get_type = NULL },
    { .path = ":sysobj/root", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = get_root, .f_get_type = NULL },
};

void gen_sysobj() {
    int i = 0;
    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }
}

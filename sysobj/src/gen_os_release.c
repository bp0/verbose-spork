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

/* Generator to convert /usr/lib/os-release into a tree
 *  :/os_release
 */
#include "sysobj.h"

#define PATH_TO_OS_RELEASE "/etc/os-release"
#define PATH_TO_LSB_RELEASE "/etc/lsb-release"

static sysobj_virt vol[] = {
    { .path = ":/os", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

void try_os_release() {
    sysobj *obj = sysobj_new_from_fn(PATH_TO_OS_RELEASE, NULL);
    if (obj->exists) {
        sysobj_read(obj, FALSE);
        sysobj_virt_add_simple(":/os/os_release", NULL, "*", VSO_TYPE_DIR);
        sysobj_virt_from_kv(":/os/os_release", obj->data.str);
    }
    sysobj_free(obj);
}

void try_lsb_release() {
    sysobj *obj = sysobj_new_from_fn(PATH_TO_LSB_RELEASE, NULL);
    if (obj->exists) {
        sysobj_read(obj, FALSE);
        sysobj_virt_add_simple(":/os/lsb_release", NULL, "*", VSO_TYPE_DIR);
        sysobj_virt_from_kv(":/os/lsb_release", obj->data.str);
    }
    sysobj_free(obj);
}

void gen_os_release() {
    int i = 0;
    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    try_os_release();
    try_lsb_release();
}

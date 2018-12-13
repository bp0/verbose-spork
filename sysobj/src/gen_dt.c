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

/* Generator prepares maps for
 *  :/devicetree
 */

#include "sysobj.h"
#include "util_dt.h"

static gchar *dt_messages(const gchar *path) {
    if (!path) {
        dtr_cleanup();
        return NULL;
    }
    return g_strdup(dtr_log ? dtr_log : "");
}

static sysobj_virt vol[] = {
    { .path = ":/devicetree", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
    { .path = ":/devicetree/base", .str = DTROOT,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST },
    { .path = ":/devicetree/_messages", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST | VSO_TYPE_CLEANUP,
      .f_get_data = dt_messages },
    { .path = ":/devicetree/_phandle_map", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
    { .path = ":/devicetree/_alias_map", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
    { .path = ":/devicetree/_symbol_map", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
};

void gen_dt() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    dtr_init(); /* cleanup via :/devicetree/_messages */
}

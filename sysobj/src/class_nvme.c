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

static attr_tab nvme_items[] = {
    { "cntlid" },
    { "firmware_rev" },
    { "model" },
    { "serial" },
    { "state" },
    { "transport" },
    ATTR_TAB_LAST
};

static sysobj_class cls_nvme[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "nvme", .pattern = "/sys/*", .flags = OF_CONST | OF_GLOB_PATTERN | OF_HAS_VENDOR,
    .v_subsystem = "/sys/class/nvme", .v_is_node = TRUE,
    .s_vendors_from_child = "device",
    .s_node_format = "{{@vendors}}{{model}}",
    },
  { SYSOBJ_CLASS_DEF
    .tag = "nvme:attr", .pattern = "/sys/*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .v_subsystem_parent = "/sys/class/nvme", .attributes = nvme_items },
};

void class_nvme() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_nvme); i++)
        class_add(&cls_nvme[i]);
}

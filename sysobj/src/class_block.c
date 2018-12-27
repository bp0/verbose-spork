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
#include "format_funcs.h"

const gchar block_stat_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/block/stat.txt")
    "\n";

static gchar *block_format(sysobj *obj, int fmt_opts);

static attr_tab block_items[] = {
    { "size", N_("size in sectors") },
    { "stat", N_("block layer statistics"), OF_NONE, NULL, 0.5 },
    { "removable", N_("is removable"), OF_NONE, fmt_1yes0no },
    { "force_ro", N_("force read-only"), OF_NONE, fmt_1yes0no },
    { "ro", N_("is read-only"), OF_NONE, fmt_1yes0no },
    { "partition", N_("partition number") },
    ATTR_TAB_LAST
};

static sysobj_class cls_block[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "block", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem = "/sys/class/block", .f_format = block_format },
  { SYSOBJ_CLASS_DEF
    .tag = "block:attr", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/class/block", .attributes = block_items },
  { SYSOBJ_CLASS_DEF
    .tag = "block:attr:stat", .pattern = "/sys/devices/*/stat", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/class/block", .attributes = block_items,
    .s_halp = block_stat_reference_markup_text },
};

static gchar *block_format(sysobj *obj, int fmt_opts) {
    return simple_format(obj, fmt_opts);
}

void class_block() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_block); i++)
        class_add(&cls_block[i]);
}

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

const gchar power_supply_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/power/power_supply_class.txt")
    "\n";

static gchar *power_supply_format(sysobj *obj, int fmt_opts);

static attr_tab power_supply_items[] = {
    { "capacity_level" },
    { "manufacturer", NULL, OF_HAS_VENDOR },
    { "model_name" },
    { "online" },
    { "scope" },
    { "serial_number" },
    { "status" },
    { "type" },
    ATTR_TAB_LAST
};

static sysobj_class cls_power_supply[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "power_supply", .pattern = "/sys/devices/*", .flags = OF_CONST | OF_GLOB_PATTERN | OF_HAS_VENDOR,
    .v_subsystem = "/sys/class/power_supply",
    .s_vendors_from_child = "manufacturer", .s_node_format = "{{@vendors!manufacturer}}{{model_name}}",
    .s_halp = power_supply_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "power_supply:attr", .pattern = "/sys/devices/*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .v_subsystem_parent = "/sys/class/power_supply", .attributes = power_supply_items,
    .s_halp = power_supply_reference_markup_text },
};

void class_power_supply() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_power_supply); i++)
        class_add(&cls_power_supply[i]);
}

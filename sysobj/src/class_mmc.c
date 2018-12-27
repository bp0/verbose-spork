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

const gchar mmc_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/mmc/mmc-dev-attrs.txt")
    "\n";

static attr_tab mmc_items[] = {
    { "force_ro", N_("enforce read-only access even if write protect switch is off"), OF_NONE, fmt_1yes0no },
    { "cid",      N_("Card Identification Register") },
    { "csd",      N_("Card Specific Data Register") },
    { "scr",      N_("SD Card Configuration Register (SD only)") },
    { "date",     N_("manufacturing date (from CID Register)") },
    { "fwrev",    N_("firmware/product revision (from CID Register) (SD and MMCv1 only)") },
    { "hwrev",    N_("hardware/product revision (from CID Register) (SD and MMCv1 only)") },
    { "manfid",   N_("manufacturer ID (from CID Register)") },
    { "name",     N_("product name (from CID Register)") },
    { "oemid",    N_("OEM/Application ID (from CID Register)") },
    { "prv",      N_("product revision (from CID Register) (SD and MMCv4 only)") },
    { "serial",               N_("product serial number (from CID Register)") },
    { "erase_size",           N_("erase group size") },
    { "preferred_erase_size", N_("preferred erase size") },
    { "raw_rpmb_size_mult",   N_("RPMB partition size") },
    { "rel_sectors",          N_("reliable write sector count") },
    { "ocr",                  N_("Operation Conditions Register") },
    { "dsr",                  N_("Driver Stage Register") },
    { "cmdq_en",              N_("Command Queue enabled"), OF_NONE, fmt_1yes0no },
    { "type" }, //TODO: SD, SDIO, ...
    ATTR_TAB_LAST
};

static sysobj_class cls_mmc[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "mmc", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem = "/sys/bus/mmc", .s_node_format = "{{type}}{{name}}",
    .s_halp = mmc_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "mmc:attr", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/mmc", .attributes = mmc_items,
    .s_halp = mmc_reference_markup_text },
};

void class_mmc() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_mmc); i++)
        class_add(&cls_mmc[i]);
}

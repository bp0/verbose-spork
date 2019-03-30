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

/* PCI Express Advanced Error Reporting */

#include "sysobj.h"

const gchar aer_reference_markup_text[] =
    "AER = PCI Express Advanced Error Reporting.\n"
    "References:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-pci-devices-aer_stats") "\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/PCI/pcieaer-howto.txt") "\n"
    ;

static attr_tab aer_items[] = {
    { "aer_dev_correctable" },
    { "aer_dev_fatal" },
    { "aer_dev_nonfatal" },
    { "aer_rootport_total_err_cor" },
    { "aer_rootport_total_err_fatal" },
    { "aer_rootport_total_err_nonfatal" },
    ATTR_TAB_LAST
};

static sysobj_class cls_aer[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "aer:attr", .pattern = "/sys/devices/*aer_*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .attributes = aer_items,
    .s_halp = aer_reference_markup_text },
};

void class_aer() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_aer); i++)
        class_add(&cls_aer[i]);
}

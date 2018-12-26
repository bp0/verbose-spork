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

static vendor_list ven_ribbon_vendors(sysobj *obj) {
    return vendor_list_concat_va(8,
        sysobj_vendors_from_fn("/sys/devices/virtual/dmi/id", NULL), //TODO: get via mobo
        sysobj_vendors_from_fn(":/mobo", NULL),
        sysobj_vendors_from_fn(":/cpuinfo", NULL),
        sysobj_vendors_from_fn(":/gpu", NULL),
        sysobj_vendors_from_fn(":/os", NULL),
        sysobj_vendors_from_fn("/sys/bus/scsi/devices", NULL),
        sysobj_vendors_from_fn(":/pci", NULL),
        sysobj_vendors_from_fn(":/usb", NULL) );
}

gchar *ven_ribbon_format(sysobj *obj, int fmt_opts) {
    vendor_list vl = ven_ribbon_vendors(obj);
    gchar *ret = vendor_list_ribbon(vl, fmt_opts);
    vendor_list_free(vl);
    return ret;
}

static sysobj_class cls_TEMPLATE[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "ven_ribbon", .pattern = ":/extra/vendor_ribbon", .flags = OF_CONST | OF_HAS_VENDOR,
    .f_format = ven_ribbon_format, .f_vendors = ven_ribbon_vendors },
};

void class_extra() {
    sysobj_virt_add_simple_mkpath(":/extra/vendor_ribbon", NULL, "He can't go to the ribbon, so he's trying to make the ribbon come to him.", VSO_TYPE_STRING);
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_TEMPLATE); i++)
        class_add(&cls_TEMPLATE[i]);
}

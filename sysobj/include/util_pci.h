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

#ifndef _UTIL_PCI_H_
#define _UTIL_PCI_H_

#include <glib.h>
#include <stdio.h>
#include <stdint.h>  /* for *int*_t types */

typedef struct {
    uint32_t vendor;
    uint32_t device;
    uint32_t sub_vendor;
    uint32_t sub_device;
    uint32_t dev_class;

    gchar *address;
    gchar *sort_key;
    gchar *vendor_str;
    gchar *device_str;
    gchar *sub_vendor_str;
    gchar *sub_device_str;
    gchar *dev_class_str;
} util_pci_id;

void util_pci_id_free(util_pci_id *s);
/* found count or -1 for error */
int util_pci_ids_lookup(util_pci_id *pcid);
int util_pci_ids_lookup_list(GSList *items);

gboolean verify_pci_addy(gchar *str); /* ????:??:??.? */

#endif

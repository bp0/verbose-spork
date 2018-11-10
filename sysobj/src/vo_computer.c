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

/* This is a remnant that will eventually go away. */

#include "sysobj.h"

static sysobj_virt vol[] = {
    { .path = ":/computer",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .str = "*",
      .f_get_data = NULL },
  { .path = ":/computer/pci",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/bus/pci/devices",
      .f_get_data = NULL },
  { .path = ":/computer/usb",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/bus/usb/devices",
      .f_get_data = NULL },
  { .path = ":/computer/usb",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/bus/usb/devices",
      .f_get_data = NULL },
  { .path = ":/computer/batteries",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/class/power_supply",
      .f_get_data = NULL },
};

void gen_computer() {
    /* add virtual sysobj */
    int i;
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }
}

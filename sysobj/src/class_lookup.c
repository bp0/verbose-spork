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

const gchar arm_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/lookup/arm.ids/{implementer}/name\n"
    " :/lookup/arm.ids/{implementer}/{part}/name\n\n"
    "Reference:\n"
    BULLET REFLINK("https://github.com/bp0/armids")
    "\n";

const gchar sdio_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/lookup/sdio.ids/{vendor}/name\n"
    " :/lookup/sdio.ids/{vendor}/{device}/name\n"
    " :/lookup/sdio.ids/C {class}/name\n\n"
    "Reference:\n"
    BULLET REFLINK("https://github.com/systemd/systemd/blob/master/hwdb/sdio.ids")
    "\n";

const gchar sdcard_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/lookup/sdcard.ids/MANID {vendor}/name\n"
    " :/lookup/sdcard.ids/OEMID {vendor}/name\n"
    "\n";

const gchar dt_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/lookup/dt.ids/{compat_element}/vendor\n"
    " :/lookup/dt.ids/{compat_element}/name\n"
    " :/lookup/dt.ids/{compat_element}/class\n"
    "\n"
    "Reference:\n"
    BULLET REFLINKT("dt.ids", "https://github.com/bp0/dtid") "\n"
    "\n";

const gchar usb_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/lookup/usb.ids/{vendor}/name\n"
    " :/lookup/usb.ids/{vendor}/{device}/name\n\n"
    "Reference:\n"
    BULLET REFLINKT("<i>The Linux USB Project</i>'s usb.ids", "http://www.linux-usb.org/")
    "\n";

static gchar *dt_ids_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_lookup[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "arm.ids", .pattern = ":/lookup/arm.ids", .flags = OF_CONST,
    .s_halp = arm_ids_reference_markup_text, .s_label = "arm.ids lookup virtual tree",
    .s_update_interval = 4.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "arm.ids:id", .pattern = ":/lookup/arm.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = arm_ids_reference_markup_text, .s_label = "arm.ids lookup result",
    .f_format = fmt_node_name },

  { SYSOBJ_CLASS_DEF
    .tag = "sdio.ids", .pattern = ":/lookup/sdio.ids", .flags = OF_CONST,
    .s_halp = sdio_ids_reference_markup_text, .s_label = "sdio.ids lookup virtual tree",
    .s_update_interval = 4.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "sdio.ids:id", .pattern = ":/lookup/sdio.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = sdio_ids_reference_markup_text, .s_label = "sdio.ids lookup result",
    .f_format = fmt_node_name },

  { SYSOBJ_CLASS_DEF
    .tag = "sdcard.ids", .pattern = ":/lookup/sdcard.ids", .flags = OF_CONST,
    .s_halp = sdcard_ids_reference_markup_text, .s_label = "sdcard.ids lookup virtual tree",
    .s_update_interval = 4.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "sdcard.ids:id", .pattern = ":/lookup/sdcard.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = sdcard_ids_reference_markup_text, .s_label = "sdcard.ids lookup result",
    .f_format = fmt_node_name },

  { SYSOBJ_CLASS_DEF
    .tag = "usb.ids", .pattern = ":/lookup/usb.ids", .flags = OF_CONST,
    .s_halp = usb_ids_reference_markup_text, .s_label = "usb.ids lookup virtual tree",
    .s_update_interval = 4.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "usb.ids:id", .pattern = ":/lookup/usb.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = usb_ids_reference_markup_text, .s_label = "usb.ids lookup result",
    .f_format = fmt_node_name },

  { SYSOBJ_CLASS_DEF
    .tag = "dt.ids", .pattern = ":/lookup/dt.ids", .flags = OF_CONST,
    .s_halp = dt_ids_reference_markup_text, .s_label = "dt.ids lookup virtual tree",
    .f_format = dt_ids_format, .s_update_interval = 4.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "dt.ids:id", .pattern = ":/lookup/dt.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = dt_ids_reference_markup_text, .s_label = "dt.ids lookup result",
    .f_format = dt_ids_format },
};

static gchar *dt_ids_format(sysobj *obj, int fmt_opts) {
    if (obj) {
        gchar *ret = NULL;
        gchar *pname = sysobj_parent_name(obj);
        if (SEQ(pname, "dt.ids")) {
            /* compat elem */
            gchar *vendor = sysobj_raw_from_fn(obj->path, "vendor");
            gchar *name = sysobj_raw_from_fn(obj->path, "name");
            gchar *cls = sysobj_raw_from_fn(obj->path, "class");

            if (vendor || name || cls) {
                if (SEQ(cls, "vendor"))
                    ret = g_strdup_printf("%s (%s)", vendor, cls);
                else if (vendor && name && cls)
                    ret = g_strdup_printf("%s %s (%s)", vendor, name, cls);
                else if (name && cls)
                    ret = g_strdup_printf("%s (%s)", name, cls);
                else if (vendor)
                    ret = g_strdup_printf("%s %s", vendor, "Unknown");
            }
            g_free(vendor);
            g_free(name);
            g_free(cls);
        }
        g_free(pname);
        if (ret)
            return ret;
    }
    return simple_format(obj, fmt_opts);
}

void class_lookup() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_lookup); i++)
        class_add(&cls_lookup[i]);
}

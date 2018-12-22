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

/*
 * Information sources:
 *   http://elinux.org/Device_Tree_Usage
 *   http://elinux.org/Device_Tree_Mysteries
 */

#include "sysobj.h"
#include "util_dt.h"

const gchar dt_reference_markup_text[] =
    "References:\n"
    BULLET REFLINK("http://elinux.org/Device_Tree_Usage") "\n"
    BULLET REFLINK("http://elinux.org/Device_Tree_Mysteries") "\n"
    "\n";

const gchar dt_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/devicetree/dt.ids/{compat_element}/vendor\n"
    " :/devicetree/dt.ids/{compat_element}/name\n"
    " :/devicetree/dt.ids/{compat_element}/class\n"
    "\n"
    "Reference:\n"
    BULLET REFLINKT("dt.ids", "https://github.com/bp0/dtid") "\n"
    "\n";

static gchar *dt_ids_format(sysobj *obj, int fmt_opts);
static vendor_list dt_compat_vendors(sysobj *obj);

static sysobj_class cls_dtr[] = {
  /* all else (first added is last tested for match) */
  { SYSOBJ_CLASS_DEF
    .tag = "devicetree", .pattern = DTROOT "*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = dt_reference_markup_text, .s_update_interval = 0.0,
    .f_format = dtr_format },
  { SYSOBJ_CLASS_DEF
    .tag = "devicetree:compat", .pattern = DTROOT "*/compatible", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_halp = dt_reference_markup_text, .s_update_interval = 0.0,
    .f_format = dtr_format, .f_vendors = dt_compat_vendors },

  { SYSOBJ_CLASS_DEF
    .tag = "dt.ids", .pattern = ":/devicetree/dt.ids", .flags = OF_CONST,
    .s_halp = dt_ids_reference_markup_text, .s_label = "dt.ids lookup virtual tree",
    .f_format = dt_ids_format,.s_update_interval = 3.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "dt.ids:id", .pattern = ":/devicetree/dt.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
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

static vendor_list dt_compat_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    gchar *compat_str_list = obj->data.str;
    gsize len = obj->data.len;
    if (compat_str_list) {
        const gchar *el = compat_str_list;
        while(el < compat_str_list + len) {
            gchar *lookup_path = g_strdup_printf(":/devicetree/dt.ids/%s", el);
            gchar *vendor = sysobj_raw_from_fn(lookup_path, "vendor");
            if (vendor) {
                const Vendor *v = vendor_match(vendor, NULL);
                if (v)
                    ret = vendor_list_append(ret, v);
            }
            g_free(vendor);
            g_free(lookup_path);
            el += strlen(el) + 1;
        }
    }
    return ret;
}

void class_dt() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_dtr); i++)
        class_add(&cls_dtr[i]);
}

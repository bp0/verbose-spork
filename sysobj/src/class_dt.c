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
};

static vendor_list dt_compat_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    gchar *compat_str_list = obj->data.str;
    gsize len = obj->data.len;
    if (compat_str_list) {
        const gchar *el = compat_str_list;
        while(el < compat_str_list + len) {
            gchar *lookup_path = g_strdup_printf(":/lookup/dt.ids/%s", el);
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

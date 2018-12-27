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

static gchar *mobo_format(sysobj *obj, int fmt_opts);
static vendor_list mobo_vendor(sysobj *obj);

static sysobj_class cls_mobo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "mobo", .pattern = ":/mobo", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .f_format = mobo_format, .s_update_interval = UPDATE_INTERVAL_NEVER,
    .s_label = N_("main system board"),
    .f_vendors = mobo_vendor },
};

static vendor_list mobo_vendor(sysobj *obj) {
    const Vendor *v = NULL;
    gchar *vs = sysobj_raw_from_fn("/sys/firmware/devicetree/base/model", NULL);
    v = vendor_match(vs, NULL);
    g_free(vs);
    return vendor_list_concat_va(3,
        sysobj_vendors_from_fn("/sys/class/dmi/id", NULL),
        vendor_list_append(NULL, v),
        sysobj_vendors_from_fn(":/raspberry_pi", NULL)  );
}

static gchar *mobo_format(sysobj *obj, int fmt_opts) {
    if (SEQ(":/mobo", obj->path)) {
        gchar *name = sysobj_raw_from_fn(":/mobo", "nice_name");
        gchar *bac = sysobj_raw_from_fn(":/mobo", "name/board_vendor/ansi_color");
        gchar *pac = sysobj_raw_from_fn(":/mobo", "name/system_vendor/ansi_color");
        if (bac || pac) {
            gchar *bvs = sysobj_raw_from_fn(":/mobo", "name/board_vendor_str");
            gchar *pvs = sysobj_raw_from_fn(":/mobo", "name/system_vendor_str");
            if (bac && bvs)
                tag_vendor(&name, 0, bvs, bac, fmt_opts);
            if (pac && pvs) {
                gchar *p = name;
                while(p) {
                    if (g_str_has_prefix(p, pvs) ) {
                        tag_vendor(&name, p-name, pvs, pac, fmt_opts);
                        break;
                    }
                    p = strchr(p, '(');
                    if (p) p++;
                }
            }
            g_free(bvs);
            g_free(pvs);
        }
        g_free(bac);
        g_free(pac);
        return name;
    }
    return simple_format(obj, fmt_opts);
}

void class_mobo() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_mobo); i++)
        class_add(&cls_mobo[i]);
}

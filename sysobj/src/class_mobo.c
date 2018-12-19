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

static sysobj_class cls_mobo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "mobo", .pattern = ":/mobo", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = mobo_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
};

/* export */
void tag_vendor(gchar **str, guint offset, const gchar *vendor_str, const char *ansi_color, int fmt_opts) {
    if (!str || !*str) return;
    if (!vendor_str || !ansi_color) return;
    gchar *work = *str, *new = NULL;
    if (g_str_has_prefix(work + offset, vendor_str)
        || strncasecmp(work + offset, vendor_str, strlen(vendor_str)) == 0) {
        gchar *cvs = format_with_ansi_color(vendor_str, ansi_color, fmt_opts);
        *(work+offset) = 0;
        new = g_strdup_printf("%s%s%s", work, cvs, work + offset + strlen(vendor_str) );
        g_free(work);
        *str = new;
        g_free(cvs);
    }
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
                    if (g_str_has_prefix(p, pvs) )
                        tag_vendor(&name, p-name, pvs, pac, fmt_opts);
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

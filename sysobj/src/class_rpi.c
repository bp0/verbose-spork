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

static gchar *rpi_format(sysobj *obj, int fmt_opts);

attr_tab rpi_items[] = {
    { "raspberry_pi",  N_("Raspberry Pi information"), OF_NONE },
    { "board_name",    N_("model"), OF_NONE },
    { "introduction",  N_("date of introduction"), OF_NONE },
    { "manufacturer",  N_("manufacturer"), OF_HAS_VENDOR },
    { "overvolt",      N_("permanent over-volt bit"), OF_NONE },
    { "pcb_revision",  N_("revision of printed circuit board"), OF_NONE },
    { "r_code",        N_("r-code"), OF_NONE },
    { "serial",        N_("serial number"), OF_NONE },
    { "spec_mem",      N_("memory (spec)"), OF_NONE },
    { "spec_soc",      N_("system-on-chip used (spec)"), OF_NONE },
    ATTR_TAB_LAST
};

static sysobj_class cls_rpi[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "rpi", .pattern = ":/raspberry_pi*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = rpi_items,
    .f_format = rpi_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
};

static gchar *rpi_format(sysobj *obj, int fmt_opts) {
    if (SEQ("raspberry_pi", obj->name)) {
        gchar *name = sysobj_raw_from_fn(":/raspberry_pi", "board_name");
        gchar *intro = sysobj_raw_from_fn(":/raspberry_pi", "introduction");
        gchar *mfgr = sysobj_raw_from_fn(":/raspberry_pi", "manufacturer");
        gchar *full = g_strdup_printf("%s (%s; %s)", name, intro, mfgr);
        g_free(name);
        g_free(intro);
        g_free(mfgr);
        return full;
    }
    if (SEQ("overvolt", obj->name)) {
        int v = atoi(obj->data.str);
        return g_strdup_printf("[%d] %s", v, v ? "Set" : "Not-set");
    }
    return simple_format(obj, fmt_opts);
}

void class_rpi() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_rpi); i++)
        class_add(&cls_rpi[i]);
}

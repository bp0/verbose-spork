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

static const gchar *rpi_label(sysobj *s);
static gchar *rpi_format(sysobj *obj, int fmt_opts);
static double rpi_update_interval(sysobj *obj);

static sysobj_class cls_rpi[] = {
  { .tag = "rpi", .pattern = ":/raspberry_pi*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_label = rpi_label, .f_format = rpi_format, .f_update_interval = rpi_update_interval },
};

static const struct { gchar *rp; gchar *lbl; int extra_flags; } rpi_items[] = {
    { "raspberry_pi",  N_("Raspberry Pi Information"), OF_NONE },
    { "board_name",    N_("Model"), OF_NONE },
    { "introduction",  N_("Introduction"), OF_NONE },
    { "manufacturer",  N_("Manufacturer"), OF_NONE },
    { "overvolt",      N_("Permanent Over-volt Bit"), OF_NONE },
    { "pcb_revision",  N_("PCB Revision"), OF_NONE },
    { "r_code",        N_("R-Code"), OF_NONE },
    { "serial",        N_("Serial Number"), OF_NONE },
    { "spec_mem",      N_("Memory (spec)"), OF_NONE },
    { "spec_soc",      N_("SOC (spec)"), OF_NONE },
    { NULL, NULL, 0 }
};

int rpi_lookup(const gchar *key) {
    int i = 0;
    while(rpi_items[i].rp) {
        if (strcmp(key, rpi_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

const gchar *rpi_label(sysobj *obj) {
    int i = rpi_lookup(obj->name);
    if (i != -1)
        return _(rpi_items[i].lbl);
    return NULL;
}

static gchar *rpi_format(sysobj *obj, int fmt_opts) {
    if (!strcmp("raspberry_pi", obj->name)) {
        gchar *name = sysobj_raw_from_fn(":/raspberry_pi", "board_name");
        gchar *intro = sysobj_raw_from_fn(":/raspberry_pi", "introduction");
        gchar *mfgr = sysobj_raw_from_fn(":/raspberry_pi", "manufacturer");
        gchar *full = g_strdup_printf("%s (%s; %s)", name, intro, mfgr);
        g_free(name);
        g_free(intro);
        g_free(mfgr);
        return full;
    }
    if (!strcmp("overvolt", obj->name)) {
        int v = atoi(obj->data.str);
        return g_strdup_printf("[%d] %s", v, v ? "Set" : "Not-set");
    }
    return simple_format(obj, fmt_opts);
}

static double rpi_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 0.0;
}

void class_rpi() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_rpi); i++) {
        class_add(&cls_rpi[i]);
    }
}

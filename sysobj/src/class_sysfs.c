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
 * https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-power
 */

#include "sysobj.h"

#define BULLET "\u2022"
#define REFLINK(URI) "<a href=\"" URI "\">" URI "</a>"
const gchar power_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-power")
    "\n";

static gboolean subsystem_verify(sysobj *s);
static gchar *subsystem_format(sysobj *obj, int fmt_opts);

static gboolean power_verify(sysobj *s);
static const gchar *power_label(sysobj *s);
static gchar *power_format(sysobj *obj, int fmt_opts);
static double power_update_interval(sysobj *obj);

static sysobj_class cls_power[] = {
  { .tag = "subsystem:class", .pattern = "/sys/class/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = subsystem_verify,
    .s_label = "sysfs sub-system", .f_format = subsystem_format, .s_update_interval = 0.0 },
  { .tag = "subsystem:bus", .pattern = "/sys/bus/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = subsystem_verify,
    .s_label = "sysfs sub-system", .f_format = subsystem_format, .s_update_interval = 0.0 },
  { .tag = "subsystem:block", .pattern = "/sys/block/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = subsystem_verify,
    .s_label = "sysfs sub-system", .f_format = subsystem_format, .s_update_interval = 0.0 },

  { .tag = "device_power:attribute", .pattern = "/sys/devices/*/power/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = power_verify, .s_halp = power_reference_markup_text,
    .f_label = power_label, .f_format = power_format, .f_update_interval = power_update_interval },
  { .tag = "device_power", .pattern = "/sys/devices/*/power", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = power_reference_markup_text,
    .f_label = power_label, .f_format = power_format, .f_update_interval = power_update_interval },
};

static gboolean subsystem_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pp = sysobj_parent_path(obj);
    if (!g_strcmp0("/sys/class", pp) )
        ret = TRUE;
    if (!g_strcmp0("/sys/bus", pp) )
        ret = TRUE;
    if (!g_strcmp0("/sys/block", pp) )
        ret = TRUE;
    g_free(pp);
    return ret;
}

static gchar *subsystem_format(sysobj *obj, int fmt_opts) {
    if (obj) {
        if (!g_strcmp0("subsystem:class", obj->cls->tag) )
            return g_strdup_printf("class:%s", obj->name);
        if (!g_strcmp0("subsystem:bus", obj->cls->tag) )
            return g_strdup_printf("bus:%s", obj->name);
        if (!g_strcmp0("subsystem:block", obj->cls->tag) )
            return g_strdup_printf("block:%s", obj->name);
    }
    return simple_format(obj, fmt_opts);
}

static const struct { gchar *item; gchar *lbl; int extra_flags; } power_items[] = {
    { "power",         N_("Device power management information"), OF_NONE },
    { "async",         N_("Allow multi-threaded system-wide power transitions"), OF_NONE },
    { "control",       N_("Run-time power management control setting"), OF_NONE },
    { NULL, NULL, 0 }
};

static int power_lookup(const gchar *key) {
    int i = 0;
    while(power_items[i].item) {
        if (strcmp(key, power_items[i].item) == 0)
            return i;
        i++;
    }
    return -1;
}

static gboolean power_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pn = sysobj_parent_name(obj);
    if (!g_strcmp0("power", pn) )
        ret = TRUE;

    g_free(pn);
    return ret;
}

const gchar *power_label(sysobj *obj) {
    int i = power_lookup(obj->name);
    if (i != -1)
        return _(power_items[i].lbl);
    return NULL;
}

static gchar *power_format(sysobj *obj, int fmt_opts) {
    if (!g_strcmp0("power", obj->name) ) {
        gchar *ret = NULL;
        gchar *async = sysobj_raw_from_fn(obj->path, "async");
        gchar *control = sysobj_raw_from_fn(obj->path, "control");
        g_strstrip(async);
        g_strstrip(control);
        ret = g_strdup_printf("async: %s, control: %s", async, control);
        g_free(async);
        g_free(control);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static double power_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 1.0;
}

void class_power() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_power); i++) {
        class_add(&cls_power[i]);
    }
}

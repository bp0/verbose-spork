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

const gchar power_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-power")
    "\n";

static gboolean subsystem_verify(sysobj *s);
static gchar *subsystem_format(sysobj *obj, int fmt_opts);

static gchar *power_format(sysobj *obj, int fmt_opts);

static gboolean power_attr_verify(sysobj *s);

static attr_tab power_items[] = {
    { "async",   N_("allow multi-threaded system-wide power transitions") },
    { "control", N_("run-time power management control setting") },
    ATTR_TAB_LAST
};

static sysobj_class cls_power[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "subsystem:class", .pattern = "/sys/class/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = subsystem_verify,
    .s_label = "sysfs sub-system", .f_format = subsystem_format, .s_update_interval = 0.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "subsystem:block", .pattern = "/sys/block/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = subsystem_verify,
    .s_label = "sysfs sub-system", .f_format = subsystem_format, .s_update_interval = 0.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "kernel:module", .pattern = "/sys/module/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = subsystem_verify,
    .s_label = "kernel module", .f_format = subsystem_format, .s_update_interval = 0.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "device_power", .pattern = "/sys/devices/*/power", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = power_reference_markup_text, .s_label = N_("device power management information"),
    .f_format = power_format, .s_update_interval = 1.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "device_power:attribute", .pattern = "/sys/devices/*/power/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = power_attr_verify, .s_halp = power_reference_markup_text,
    .attributes = power_items, .s_update_interval = 1.0 },
};

static gboolean subsystem_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pp = sysobj_parent_path(obj);
    if (SEQ("/sys/class", pp) )
        ret = TRUE;
    if (SEQ("/sys/block", pp) )
        ret = TRUE;
    if (SEQ("/sys/module", pp) )
        ret = TRUE;
    g_free(pp);
    return ret;
}

static gchar *subsystem_format(sysobj *obj, int fmt_opts) {
    if (obj) {
        if (SEQ("subsystem:class", obj->cls->tag) )
            return g_strdup_printf("class:%s", obj->name);
        if (SEQ("subsystem:block", obj->cls->tag) )
            return g_strdup_printf("block:%s", obj->name);
        if (SEQ("kernel:module", obj->cls->tag) )
            return g_strdup_printf("kmod:%s", obj->name);
    }
    return simple_format(obj, fmt_opts);
}

/* power/ children */
static gboolean power_attr_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pn = sysobj_parent_name(obj);
    if (SEQ("power", pn) )
        ret = TRUE;
    g_free(pn);
    return ret;
}

static gchar *power_format(sysobj *obj, int fmt_opts) {
    if (SEQ("power", obj->name) ) {
        gchar *ret = NULL;
        gchar *async = sysobj_raw_from_fn(obj->path, "async");
        gchar *control = sysobj_raw_from_fn(obj->path, "control");
        if (async) g_strstrip(async);
        if (control) g_strstrip(control);
        ret = g_strdup_printf("async: %s, control: %s", async ? async : "(disabled)", control ? control : "(auto)");
        g_free(async);
        g_free(control);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

void class_power() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_power); i++)
        class_add(&cls_power[i]);
}

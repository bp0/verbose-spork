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

const gchar cpu_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-system-cpu") "\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-devices-system-cpu") "\n"
    "\n";

/* export */
gboolean cpu_verify_child(sysobj *obj) {
    return verify_lblnum_child(obj, "cpu");
}

gchar *cpu_format(sysobj *obj, int fmt_opts) {
    if (obj) {
        /* TODO: more description */
        int logical = util_get_did(obj->name, "cpu");
        gchar *lstr = g_strdup_printf("{L:%d}", logical);

        gchar *topo_str =
            sysobj_format_from_fn(obj->path, "topology", fmt_opts | FMT_OPT_SHORT | FMT_OPT_OR_NULL );
        gchar *freq_str =
            sysobj_format_from_fn(obj->path, "cpufreq", fmt_opts | FMT_OPT_SHORT | FMT_OPT_OR_NULL );

        gchar *ret = g_strdup_printf(_("CPU %s %s"),
            topo_str ? topo_str : lstr,
            freq_str ? freq_str : "");

        g_free(topo_str);
        g_free(freq_str);
        g_free(lstr);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static attr_tab cpu_list_items[] = {
    { "kernel_max", N_("the maximum cpu index allowed by the kernel configuration") },
    { "offline", N_("cpus offline because hotplugged off or exceed kernel_max") },
    { "online", N_("cpus online and being scheduled") },
    { "possible", N_("cpus allocated resources and can be brought online if present") },
    { "present", N_("cpus identified as being present in the system") },
    { "isolated" },
    ATTR_TAB_LAST
};

static attr_tab cpu_items[] = {
    { "online", N_("is online"), OF_NONE, fmt_1yes0no },
    ATTR_TAB_LAST
};

static sysobj_class cls_cpu[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "cpu_list", .pattern = "/sys/devices/system/cpu", .flags = OF_CONST,
    .v_is_node = TRUE, .s_suggest = ":/cpu", .s_halp = cpu_reference_markup_text,
    .s_update_interval = 1.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "cpu_list:attr", .pattern = "/sys/devices/system/cpu/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = cpu_list_items, .v_parent_path_suffix = "/sys/devices/system/cpu",
    .s_halp = cpu_reference_markup_text, .s_update_interval = 1.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "cpu", .pattern = "/sys/devices/system/cpu/cpu*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_is_node = TRUE, .v_subsystem = "/sys/bus/cpu", .v_lblnum = "cpu",
    .s_label = N_("logical CPU"), .s_halp = cpu_reference_markup_text,
    .f_format = cpu_format, .s_update_interval = 1.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "cpu:attr", .pattern = "/sys/devices/system/cpu/cpu*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/cpu", .v_lblnum_child = "cpu",
    .attributes = cpu_items,
    .s_halp = cpu_reference_markup_text,
    .s_update_interval = 1.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "cpu:microcode_version", .pattern = "/sys/devices/system/cpu/cpu*/microcode/version", OF_GLOB_PATTERN | OF_CONST | OF_REQ_ROOT,
    .v_is_attr = TRUE, .s_label = N_("microcode version"), .s_halp = cpu_reference_markup_text },
};

void class_cpu() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_cpu); i++)
        class_add(&cls_cpu[i]);
}

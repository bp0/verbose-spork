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

#define BULLET "\u2022"
#define REFLINK(URI) "<a href=\"" URI "\">" URI "</a>"
const gchar cpu_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-system-cpu") "\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-devices-system-cpu") "\n"
    "\n";

gboolean cpu_verify(sysobj *obj) {
    return verify_lblnum(obj, "cpu");
}

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

double cpu_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 1.0;
}

gchar *cpu_format_1yes0no(sysobj *obj, int fmt_opts) {
    if (obj && obj->data.str) {
        int value = strtol(obj->data.str, NULL, 10);
        return g_strdup_printf("[%d] %s", value, value ? _("Yes") : _("No") );
    }
    return simple_format(obj, fmt_opts);
}

static sysobj_class cls_cpu[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "cpu_list", .pattern = "/sys/devices/system/cpu", .flags = OF_CONST,
    .s_suggest = ":/procs", .s_halp = cpu_reference_markup_text,
    .f_update_interval = cpu_update_interval },

  { SYSOBJ_CLASS_DEF
    .tag = "cpu", .pattern = "/sys/devices/system/cpu/cpu*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("Logical CPU"), .s_halp = cpu_reference_markup_text,
    .f_verify = cpu_verify, .f_format = cpu_format, .f_update_interval = cpu_update_interval },

  { SYSOBJ_CLASS_DEF
    .tag = "cpu:isonline", .pattern = "/sys/devices/system/cpu/cpu*/online", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("Is Online"), .s_halp = cpu_reference_markup_text,
    .f_verify = cpu_verify_child, .f_format = cpu_format_1yes0no, .f_update_interval = cpu_update_interval },

  { SYSOBJ_CLASS_DEF
    .tag = "cpu:microcode_version", .pattern = "/sys/devices/system/cpu/cpu*/microcode/version", OF_GLOB_PATTERN | OF_CONST | OF_REQ_ROOT,
    .s_label = N_("Version"), .s_halp = cpu_reference_markup_text },
};

void class_cpu() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_cpu); i++) {
        class_add(&cls_cpu[i]);
    }
}

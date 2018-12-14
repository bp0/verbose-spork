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

const gchar cpufreq_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/cpu-freq/")
    "\n";

attr_tab cpufreq_items[] = {
    { "scaling_min_freq", N_("minimum clock frequency (via scaling driver)"), OF_NONE, fmt_khz_to_mhz, 1.0 },
    { "scaling_max_freq", N_("maximum clock frequency (via scaling driver)"), OF_NONE, fmt_khz_to_mhz, 1.0 },
    { "scaling_cur_freq", N_("current clock frequency (via scaling driver)"), OF_NONE, fmt_khz_to_mhz, 0.25 },

    { "bios_limit", N_("maximum clock frequency (via BIOS)"), OF_NONE, fmt_khz_to_mhz, -1 },

    { "cpuinfo_min_freq", N_("minimum clock frequency (via cpuinfo)"), OF_NONE, fmt_khz_to_mhz, 1.0 },
    { "cpuinfo_max_freq", N_("maximum clock frequency (via cpuinfo)"), OF_NONE, fmt_khz_to_mhz, 1.0 },
    { "cpuinfo_cur_freq", N_("current clock frequency (via cpuinfo)"), OF_NONE, fmt_khz_to_mhz, 0.25 },

    { "cpuinfo_transition_latency", N_("transition latency"), OF_NONE, fmt_nanoseconds, 2.0 },

    { "freqdomain_cpus", N_("logical CPUs that share the same base clock") },
    { "affected_cpus",   N_("logical CPUs affected by change to scaling_setspeed") },
    { "related_cpus",    N_("related logical CPUs") },
    ATTR_TAB_LAST
};

gboolean cpufreq_verify(sysobj *obj) {
    /* child of "policy%d" */
    if (verify_lblnum_child(obj, "policy")
        && verify_in_attr_tab(obj, cpufreq_items) )
        return TRUE;
    return FALSE;
}

gboolean cpufreq_verify_policy(sysobj *obj) {
    return verify_lblnum(obj, "policy");
}

gchar *cpufreq_format_policy(sysobj *obj, int fmt_opts) {
    if (obj) {
        gchar *ret = NULL;
        /* get nice description */
        gchar *scl_min =
            sysobj_format_from_fn(obj->path, "scaling_min_freq", fmt_opts | FMT_OPT_PART | FMT_OPT_NO_UNIT );
        gchar *scl_max =
            sysobj_format_from_fn(obj->path, "scaling_max_freq", fmt_opts | FMT_OPT_PART);
        gchar *scl_gov =
            sysobj_format_from_fn(obj->path, "scaling_governor", fmt_opts | FMT_OPT_PART);
        gchar *scl_drv =
            sysobj_format_from_fn(obj->path, "scaling_driver", fmt_opts | FMT_OPT_PART);
        gchar *scl_cur =
            sysobj_format_from_fn(obj->path, "scaling_cur_freq", fmt_opts | FMT_OPT_PART);

        if (fmt_opts & FMT_OPT_SHORT)
            ret = g_strdup_printf("%s", scl_cur );
        else
            ret = g_strdup_printf("%s %s (%s - %s) [ %s ]",
                    scl_drv, scl_gov, util_strchomp_float(scl_min), util_strchomp_float(scl_max), scl_cur );

        g_free(scl_min);
        g_free(scl_max);
        g_free(scl_gov);
        g_free(scl_drv);
        g_free(scl_cur);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static sysobj_class cls_cpufreq[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "cpufreq:policy", .pattern = "/sys/devices/system/cpu/cpufreq/policy*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = cpufreq_reference_markup_text, .s_label = N_("frequency scaling policy"),
    .f_verify = cpufreq_verify_policy, .f_format = cpufreq_format_policy, .s_update_interval = 0.25 },
  { SYSOBJ_CLASS_DEF
    .tag = "cpufreq:item", .pattern = "/sys/devices/system/cpu/cpufreq/policy*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = cpufreq_reference_markup_text,
    .f_verify = cpufreq_verify, .attributes = cpufreq_items },
};

void class_cpufreq() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_cpufreq); i++)
        class_add(&cls_cpufreq[i]);
}

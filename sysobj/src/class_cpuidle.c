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

const gchar cpuidle_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/cpuidle/sysfs.txt")
    "\n";

static attr_tab cpuidle_items[] = {
    { "current_driver", N_("cpuidle driver name") },
    { "current_governor_ro", N_("actual governor") },
    { "current_governor", N_("requested governor") },
    { "available_governors" },
    ATTR_TAB_LAST
};

static gchar *state_name_format(sysobj *obj, int fmt_opts) {
    if (obj->data.str)
        return g_strstrip(g_strdup(obj->data.str));
    return simple_format(obj, fmt_opts);
}

/* obj is cpuN/cpuidle */
static int state_count(sysobj *obj) {
    int ret = 0;
    GSList *childs = sysobj_children(obj, "state*", NULL, FALSE);
    ret = g_slist_length(childs);
    g_slist_free_full(childs, g_free);
    return ret;
}

static gchar *cpuidle_cpu_format(sysobj *obj, int fmt_opts) {
    //TODO: handle fmt_opts
    gchar *ret = NULL;
    gchar *drv = sysobj_format_from_fn(obj->path, "driver", fmt_opts);
    gchar *states = NULL;
    int c = state_count(obj);
    const char *fmt = ngettext("%d state", "%d states", c);
    states = g_strdup_printf(fmt, c);
    if (drv)
        ret = g_strdup_printf("%s: %s", drv, states);
    else
        ret = g_strdup(states);
    g_free(states);
    g_free(drv);
    return ret;
}

static attr_tab state_items[] = {
    { "desc", N_("small description about the idle state") },
    { "disable", N_("option to disable this idle state"), OF_NONE, fmt_1yes0no },
    { "latency", N_("latency to exit out of this idle state"), OF_NONE, fmt_microseconds_to_milliseconds },
    { "residency", N_("time after which a state becomes more effecient than any shallower state"), OF_NONE, fmt_microseconds_to_milliseconds },
    { "name", N_("name of the idle state"), OF_NONE, state_name_format },
    { "power", N_("power consumed while in this idle state"), OF_NONE, fmt_milliwatt },
    { "time", N_("total time spent in this idle state"), OF_NONE, fmt_microseconds_to_milliseconds },
    { "usage", N_("number of times this state was entered") },
    ATTR_TAB_LAST
};

static sysobj_class cls_cpuidle[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "cpuidle", .pattern = "/sys/devices/system/cpu/cpuidle", .flags = OF_CONST,
    .v_is_node = TRUE, .s_node_format = "{{current_driver}}{{current_governor_ro}}{{current_governor}}",
    .s_halp = cpuidle_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuidle:attr", .pattern = "/sys/devices/system/cpu/cpuidle/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = cpuidle_items, .v_parent_path_suffix = "/sys/devices/system/cpu/cpuidle",
    .s_halp = cpuidle_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuidle:cpu", .pattern = "/sys/devices/*/cpu*/cpuidle", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum_child = "cpu", .f_format = cpuidle_cpu_format,
    .s_halp = cpuidle_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuidle:cpu:driver", .pattern = "/sys/devices/*/cpu*/cpuidle/driver", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_is_node = TRUE, .f_format = fmt_node_name, .s_halp = cpuidle_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuidle:cpu:state", .pattern = "/sys/devices/*/cpu*/cpuidle/state*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum = "state", .s_node_format = "{{name}}{{: |desc}}",
    .s_halp = cpuidle_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuidle:cpu:state:attr", .pattern = "/sys/devices/*/cpu*/cpuidle/state*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum_child = "state", .attributes = state_items,
    .s_halp = cpuidle_reference_markup_text },
};

void class_cpuidle() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_cpuidle); i++)
        class_add(&cls_cpuidle[i]);
}

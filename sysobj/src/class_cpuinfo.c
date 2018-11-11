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
#include "arm_data.h"
#include "x86_data.h"

static gchar *cpuinfo_feature_format(sysobj *obj, int fmt_opts);
static gchar *cpuinfo_format(sysobj *obj, int fmt_opts);
static double cpuinfo_update_interval(sysobj *obj);

static sysobj_class cls_cpuinfo[] = {
  { .tag = "cpuinfo/feature", .pattern = ":/cpuinfo/*/flags/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_feature_format, .f_update_interval = cpuinfo_update_interval },
  { .tag = "cpuinfo/featurelist", .pattern = ":/cpuinfo/*/flags", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_format, .f_update_interval = cpuinfo_update_interval },
  /* all else */
  { .tag = "cpuinfo", .pattern = ":/cpuinfo/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_format, .f_update_interval = cpuinfo_update_interval },
};

static gchar *cpuinfo_feature_format(sysobj *obj, int fmt_opts) {
    const gchar *meaning = NULL;
    gchar *type_str = sysobj_format_from_fn(obj->path, "../../_type", FMT_OPT_PART);

    PARAM_NOT_UNUSED(fmt_opts);

    if (!strcmp(type_str, "x86"))
        meaning = x86_flag_meaning(obj->data.str); /* returns translated */

    if (!strcmp(type_str, "arm"))
        meaning = arm_flag_meaning(obj->data.str); /* returns translated */

    if (meaning)
        return g_strdup_printf("%s", meaning);
    return g_strdup("");
}

static gchar *cpuinfo_format(sysobj *obj, int fmt_opts) {
    if (verify_lblnum(obj, "logical_cpu") ) {
        return sysobj_raw_from_fn(obj->path, "model_name");
    }
    return simple_format(obj, fmt_opts);
}

static double cpuinfo_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 0.0;
}

void class_cpuinfo() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_cpuinfo); i++) {
        class_add(&cls_cpuinfo[i]);
    }
}

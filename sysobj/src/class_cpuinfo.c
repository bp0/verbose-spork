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
  { .tag = "cpuinfo", .pattern = ":/cpuinfo*", .flags = OF_GLOB_PATTERN | OF_CONST,
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

static gchar *cpuinfo_describe_models() {
    sysobj *obj = sysobj_new_from_fn(":/cpuinfo", NULL);
    gchar *ret = NULL;
    GSList *models = NULL, *l = NULL;
    GSList *childs = sysobj_children(obj, "logical_cpu*", NULL, FALSE);
    if (!childs) return NULL;
    for (l = childs; l; l = l->next) {
        sysobj *co = sysobj_new_from_fn(obj->path, (gchar *)l->data);
        models = g_slist_append(models, sysobj_raw_from_fn(co->path, "model_name") );
        sysobj_free(co);
    }
    g_slist_free_full(childs, g_free);
    models = g_slist_sort(models, (GCompareFunc)g_strcmp0);
    gchar *cur_str = NULL;
    gint cur_count = 0;
    for (l = models; l; l = l->next) {
        gchar *model = (gchar*)l->data;
        if (cur_str == NULL) {
            cur_str = model;
            cur_count = 1;
        } else {
            if(g_strcmp0(cur_str, model) ) {
                ret = appfs(ret, " + ", "%dx %s", cur_count, cur_str);
                cur_str = model;
                cur_count = 1;
            } else
                cur_count++;
        }
    }
    ret = appfs(ret, " + ", "%dx %s", cur_count, cur_str);
    g_slist_free_full(models, g_free);
    sysobj_free(obj);
    return ret;
}

static gchar *cpuinfo_format(sysobj *obj, int fmt_opts) {
    if (verify_lblnum(obj, "logical_cpu") )
        return sysobj_raw_from_fn(obj->path, "model_name");
    if (!strcmp(obj->name, "cpuinfo") ) {
        gchar *ret = cpuinfo_describe_models();
        if (ret) return ret;
        return g_strdup(_("(Unknown)"));
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

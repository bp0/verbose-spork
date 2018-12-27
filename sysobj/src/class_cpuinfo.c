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
#include "sysobj_extras.h"
#include "format_funcs.h"
#include "arm_data.h"
#include "x86_data.h"
#include "riscv_data.h"

const gchar arm_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/cpu/cpuinfo/arm.ids/{implementer}/name\n"
    " :/cpu/cpuinfo/arm.ids/{implementer}/{part}/name\n"
    "Reference:\n"
    BULLET REFLINK("https://github.com/bp0/armids")
    "\n";

static gboolean cpuinfo_lcpu_verify(sysobj *obj);
static gchar *cpuinfo_feature_format(sysobj *obj, int fmt_opts);
static gchar *cpuinfo_format(sysobj *obj, int fmt_opts);

static gboolean cpuinfo_lcpu_prop_verify(sysobj *obj);

static gchar *fmt_arm_implementer(sysobj *obj, int fmt_opts) {
    int imp = strtol(obj->data.str, NULL, 16);
    gchar *istr = auto_free( sysobj_raw_from_printf(":/cpu/cpuinfo/arm.ids/%02x/name", imp) );
    if (istr)
        return g_strdup_printf("[%s] %s", obj->data.str, istr);
    simple_format(obj, fmt_opts);
}

static gchar *fmt_arm_part(sysobj *obj, int fmt_opts) {
    sysobj *obj_imp = sysobj_sibling(obj, "cpu_implementer", FALSE);
    auto_free_ex(obj_imp, (GDestroyNotify)sysobj_free);
    if (obj_imp->exists) {
        sysobj_read(obj_imp, FALSE);
        int imp = strtol(obj_imp->data.str, NULL, 16);
        int part = strtol(obj->data.str, NULL, 16);
        gchar *pstr = auto_free( sysobj_raw_from_printf(":/cpu/cpuinfo/arm.ids/%02x/%03x/name", imp, part) );
        if (pstr)
            return g_strdup_printf("[%s] %s", obj->data.str, pstr);
    }
    simple_format(obj, fmt_opts);
}

static gchar *fmt_arm_arch(sysobj *obj, int fmt_opts) {
    const gchar *aam = arm_arch_more(obj->data.str);
    if (aam)
        return g_strdup_printf("[%s] %s", obj->data.str, aam);
    simple_format(obj, fmt_opts);
}

static attr_tab cpu_prop_items[] = {
    { "cpu_implementer", NULL, OF_HAS_VENDOR, fmt_arm_implementer },
    { "cpu_part", NULL, OF_NONE, fmt_arm_part },
    { "cpu_architecture", NULL, OF_NONE, fmt_arm_arch },
    { "cpu_variant" },
    { "cpu_revision" },
    { "vendor_id", NULL, OF_HAS_VENDOR },
    ATTR_TAB_LAST
};

static vendor_list cpuinfo_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    GSList *childs = sysobj_children(obj, "logical_cpu*", NULL, TRUE);
    for(GSList *l = childs; l; l = l->next)
        ret = vendor_list_concat(ret, sysobj_vendors_from_fn(obj->path, l->data));
    g_slist_free_full(childs, g_free);
    return ret;
}

static vendor_list cpuinfo_lcpu_vendors(sysobj *obj) {
    return vendor_list_concat(
        /* x86 */ sysobj_vendors_from_fn(obj->path, "vendor_id"),
        /* arm */ sysobj_vendors_from_fn(obj->path, "cpu_implementer") );
}

static vendor_list cpuinfo_lcpu_prop_vendors(sysobj *obj) {
    if (SEQ(obj->name, "cpu_implementer") ) {
        int imp = strtol(obj->data.str, NULL, 16);
        gchar *istr = auto_free( sysobj_raw_from_printf(":/cpu/cpuinfo/arm.ids/%02x/name", imp) );
        return vendor_list_append(NULL, vendor_match(istr, NULL) );
    }
    return simple_vendors(obj);
}

static sysobj_class cls_cpuinfo[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo", .pattern = ":/cpu/cpuinfo", .flags = OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("CPU information from /proc/cpuinfo"),
    .f_format = cpuinfo_format, .s_update_interval = UPDATE_INTERVAL_NEVER,
    .f_vendors = cpuinfo_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:lcpu", .pattern = ":/cpu/cpuinfo/logical_cpu*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .v_lblnum = "logical_cpu", .f_vendors = cpuinfo_lcpu_vendors,
    .f_format = cpuinfo_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:lcpu:prop", .pattern = ":/cpu/cpuinfo/logical_cpu*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum_child = "logical_cpu", .attributes = cpu_prop_items, .f_vendors = cpuinfo_lcpu_prop_vendors,
    .s_update_interval = UPDATE_INTERVAL_NEVER },

  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:flag", .pattern = ":/cpu/cpuinfo/*/flags/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_feature_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:flag:list", .pattern = ":/cpu/cpuinfo/*/flags", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:isa", .pattern = ":/cpu/cpuinfo/*/isa/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_feature_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:isa:list", .pattern = ":/cpu/cpuinfo/*/isa", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:feature", .pattern = ":/cpu/cpuinfo/*/features/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_feature_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "cpuinfo:feature:list", .pattern = ":/cpu/cpuinfo/*/features", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = cpuinfo_format, .s_update_interval = UPDATE_INTERVAL_NEVER },

  { SYSOBJ_CLASS_DEF
    .tag = "arm.ids", .pattern = ":/cpu/cpuinfo/arm.ids", .flags = OF_CONST,
    .s_halp = arm_ids_reference_markup_text, .s_label = "arm.ids lookup virtual tree",
    .s_update_interval = 4.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "arm.ids:id", .pattern = ":/cpu/cpuinfo/arm.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = arm_ids_reference_markup_text, .s_label = "arm.ids lookup result",
    .f_format = fmt_node_name },
};

static gchar *cpuinfo_feature_format(sysobj *obj, int fmt_opts) {
    const gchar *meaning = NULL;
    gchar *type_str = sysobj_format_from_fn(obj->path, "../../arch_family", FMT_OPT_PART);

    PARAM_NOT_UNUSED(fmt_opts);

    if (SEQ(type_str, "x86"))
        meaning = x86_flag_meaning(obj->data.str); /* returns translated */

    if (SEQ(type_str, "arm"))
        meaning = arm_flag_meaning(obj->data.str); /* returns translated */

    if (SEQ(type_str, "risc-v"))
        meaning = riscv_ext_meaning(obj->data.str); /* returns translated */

    if (meaning)
        return g_strdup_printf("%s", meaning);
    return g_strdup("");
}

static gchar *cpuinfo_describe_models(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    GSList *models = NULL, *l = NULL;
    GSList *childs = sysobj_children(obj, "logical_cpu*", NULL, FALSE);
    if (!childs) return NULL;
    for (l = childs; l; l = l->next) {
        models = g_slist_append(models, sysobj_format_from_fn(obj->path, (gchar*)l->data, fmt_opts) );
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
    return ret;
}

static gchar *cpuinfo_format(sysobj *obj, int fmt_opts) {
    if (verify_lblnum(obj, "logical_cpu") ) {
        gchar *name = sysobj_raw_from_fn(obj->path, "model_name");
        str_shorten(name, "Intel(R)", "Intel");
        util_compress_space(name);
        gchar *vac = sysobj_raw_from_fn(obj->path, "vendor/ansi_color");
        if (vac) {
            gchar *vs = sysobj_raw_from_fn(obj->path, "vendor/name_short");
            if (!vs)
                vs = sysobj_raw_from_fn(obj->path, "vendor/name");
            if (vs) {
                tag_vendor(&name, 0, vs, vac, fmt_opts);
                g_free(vs);
            }
        }
        g_free(vac);
        return name;
    }
    if (SEQ(obj->name, "cpuinfo") ) {
        gchar *ret = cpuinfo_describe_models(obj, fmt_opts);
        if (ret)
            return ret;
        return g_strdup(_("(Unknown)"));
    }
    return simple_format(obj, fmt_opts);
}

void class_cpuinfo() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_cpuinfo); i++)
        class_add(&cls_cpuinfo[i]);
}

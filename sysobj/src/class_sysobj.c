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

#define CREATE_VENDOR_IDS_LOOKUP 1

gboolean class_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pp = sysobj_parent_path(obj);
    if (SEQ(pp, ":sysobj/classes") )
        ret = TRUE;
    g_free(pp);
    return ret;
}

gchar *class_format(sysobj *obj, int fmt_opts) {
    gchar *file = auto_free(sysobj_raw_from_fn(obj->path, ".def_file") );
    gchar *line = auto_free(sysobj_raw_from_fn(obj->path, ".def_line") );
    if (file && line) {
        gchar *ret = g_strdup_printf("%s:%s", file, line);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

gchar *class_flags_format(sysobj *obj, int fmt_opts) {
    gchar *flags_list = NULL;
    uint32_t flags = strtol(obj->data.str, NULL, 16);
    if (flags & OF_CONST)
        flags_list = appfs(flags_list, " | ", "%s", "OF_CONST");
    if (flags & OF_BLAST)
        flags_list = appfs(flags_list, " | ", "%s", "OF_BLAST");
    if (flags & OF_GLOB_PATTERN)
        flags_list = appfs(flags_list, " | ", "%s", "OF_GLOB_PATTERN");

    if (flags & OF_REQ_ROOT)
        flags_list = appfs(flags_list, " | ", "%s", "OF_REQ_ROOT");
    if (flags & OF_HAS_VENDOR)
        flags_list = appfs(flags_list, " | ", "%s", "OF_HAS_VENDOR");

    if (flags_list) {
        gchar *ret = g_strdup_printf("[%s] %s", obj->data.str, flags_list);
        g_free(flags_list);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static attr_tab sysobj_items[] = {
    { "root", N_("the alternate filesystem root, if any") },
    { "elapsed", N_("seconds since sysobj_init()"), OF_NONE, fmt_seconds_to_span, 0.2 },
    { "class_count", N_("number of sysobj classes defined") },
    { "classify_none", N_("sysobj_classify() found none") },
    { "class_iter", N_("steps through the class list") },
    { "classify_pattern_cmp" },
    { "virt_count", N_("number of objects in the sysobj virtual tree") },
    { "vo_tree_count" },
    { "vo_list_count" },
    { "virt_iter", N_("steps through the dynamic virtual object list") },
    { "virt_rm" },
    { "virt_add" },
    { "virt_replace" },
    { "virt_fget" },
    { "virt_fget_bytes", NULL, OF_NONE, fmt_bytes_to_higher },
    { "virt_fset" },
    { "virt_fset_bytes", NULL, OF_NONE, fmt_bytes_to_higher },
    { "free_expected", N_("seconds until next free_auto_free()"), OF_NONE, fmt_seconds, 0.2 },
    { "freed_count", N_("memory objects freed via auto_free()") },
    { "free_queue", N_("memory objects waiting to be freed via auto_free()") },
    { "free_delay", N_("minimum time until free() from auto_free()"), OF_NONE, fmt_seconds, UPDATE_INTERVAL_NEVER },
    { "sysobj_new", N_("sysobj created with sysobj_new()") },
    { "sysobj_new_fast", N_("sysobj created without sysobj_classify()") },
    { "sysobj_clean", N_("sysobj cleared") },
    { "sysobj_free", N_("sysobj freed") },
    { "gg_file_total_wait", N_("time spent waiting for read() in gg_file_get_contents_non_blocking()"), OF_NONE, fmt_microseconds_to_milliseconds },
    { "sysobj_read_first" },
    { "sysobj_read_force" },
    { "sysobj_read_expired" },
    { "sysobj_read_not_expired" },
    { "sysobj_read_wo" },
    { "sysobj_read_bytes", NULL, OF_NONE, fmt_bytes_to_higher },
    { "ven_iter", N_("steps through the vendors list") },
    { "filter_iter" },
    { "filter_pattern_cmp" },
    ATTR_TAB_LAST
};


static gboolean verify_ansi_color(sysobj *obj) {
    sysobj_read(obj, FALSE);
    gchar *sf = safe_ansi_color(obj->data.str, FALSE);
    if (sf) {
        g_free(sf);
        return TRUE;
    }
    return FALSE;
}

static gchar *format_ansi_color(sysobj *obj, int fmt_opts) {
    return format_with_ansi_color(obj->data.str, obj->data.str, fmt_opts);
}

static sysobj_class cls_sysobj[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "vsfs", .pattern = ":", .flags = OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("Virtual sysfs root"), .s_update_interval = 60.0,
    .s_vendors_from_child = "extra/vendor_ribbon" },
  { SYSOBJ_CLASS_DEF
    .tag = "extern", .pattern = ":/extern", .flags = OF_CONST,
    .s_label = N_("items that may depend on external utilities") },
  { SYSOBJ_CLASS_DEF
    .tag = "lookup", .pattern = ":/lookup", .flags = OF_CONST,
    .s_label = N_("lookup cache") },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:stat", .pattern = ":sysobj/*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .attributes = sysobj_items, .s_update_interval = 2.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:class", .pattern = ":sysobj/classes/*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .f_verify = class_verify, .f_format = class_format },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:class:flags", .pattern = ":sysobj/classes/*/.flags", .flags = OF_CONST | OF_GLOB_PATTERN,
    .f_format = class_flags_format },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:class:hits", .pattern = ":sysobj/classes/*/hits", .flags = OF_CONST | OF_GLOB_PATTERN,
    .s_update_interval = 0.5 },
  { SYSOBJ_CLASS_DEF
    .tag = "ansi_color", .pattern = "*/ansi_color", .flags = OF_CONST | OF_GLOB_PATTERN,
    .f_verify = verify_ansi_color, .f_format = format_ansi_color },

  { SYSOBJ_CLASS_DEF
    .tag = "vendor:search", .pattern = ":/lookup/vendor.ids/*", .flags = OF_CONST | OF_GLOB_PATTERN | OF_HAS_VENDOR,
    .v_parent_path_suffix = ":/lookup/vendor.ids", .s_node_format = "{{@vendors}}{{: |name}}{{: |url}}",
    .s_vendors_from_child = "match_string" },
  { SYSOBJ_CLASS_DEF
    .tag = "vendor:search:attr", .pattern = ":/lookup/vendor.ids/*/match_string",
    .flags = OF_CONST | OF_GLOB_PATTERN | OF_HAS_VENDOR  }
};

/* in vendor.c */
const GSList *get_vendors_list();

void make_vendors_lookup() {
    gchar *vlpath = ":/lookup/vendor.ids/";
    sysobj_virt_add_simple(vlpath, NULL, "*", VSO_TYPE_DIR);
    const GSList *vl = get_vendors_list();
    for (const GSList *l = vl; l; l = l->next) {
        const Vendor *v = l->data;
        gchar lnb[128] = "";
        gchar *safe_ms = util_safe_name(v->match_string, FALSE);
        gchar *mspath = util_build_fn(vlpath, safe_ms);
        sysobj_virt_add_simple(mspath, NULL, "*", VSO_TYPE_DIR);
        sysobj_virt_add_simple(mspath, "name", v->name, VSO_TYPE_STRING);
        sysobj_virt_add_simple(mspath, "name_short", v->name_short, VSO_TYPE_STRING);
        sysobj_virt_add_simple(mspath, "url", v->url, VSO_TYPE_STRING);
        sysobj_virt_add_simple(mspath, "url_support", v->url_support, VSO_TYPE_STRING);
        sysobj_virt_add_simple(mspath, "ansi_color", v->ansi_color, VSO_TYPE_STRING);
        sysobj_virt_add_simple(mspath, "match_string", v->match_string, VSO_TYPE_STRING);
        sysobj_virt_add_simple(mspath, "match_case", v->match_case ? "1" : "0", VSO_TYPE_STRING);
        sprintf(lnb, "%lu", v->file_line);
        sysobj_virt_add_simple(mspath, "file_line", lnb, VSO_TYPE_STRING);
        g_free(mspath);
        g_free(safe_ms);
    }
}

void class_sysobj() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_sysobj); i++)
        class_add(&cls_sysobj[i]);

    if (CREATE_VENDOR_IDS_LOOKUP)
        make_vendors_lookup();
}

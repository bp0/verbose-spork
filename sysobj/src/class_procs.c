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

static gchar *procs_format(sysobj *obj, int fmt_opts);
static vendor_list procs_vendors(sysobj *obj) { return sysobj_vendors_from_fn(obj->path, "cpuinfo"); }

static attr_tab procs_items[] = {
    { "packs", N_("number of sockets/packages/clusters discovered") },
    { "cores", N_("number of cores discovered") },
    { "threads", N_("number of hardware threads discovered") },
    { "freq_domains", N_("number of scaling units discovered") },
    { "soc_name", N_("model name of system-on-chip") },
    { "soc_vendor", N_("vendor name of system-on-chip"), OF_HAS_VENDOR, fmt_vendor_name_to_tag },
    { "topo_desc", NULL, OF_NONE, procs_format },
    ATTR_TAB_LAST
};

static sysobj_class cls_procs[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "procs", .pattern = ":/cpu", .flags = OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("processor information"),
    .f_format = procs_format, .s_update_interval = UPDATE_INTERVAL_NEVER, .f_vendors = procs_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "procs:pack", .pattern = ":/cpu/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum = "package", .v_parent_path_suffix = ":/cpu",
    .f_format = procs_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "procs:core", .pattern = ":/cpu/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum = "core", .v_lblnum_child = "package",
    .f_format = procs_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "procs:thread", .pattern = ":/cpu/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum = "thread", .v_lblnum_child = "core",
    .f_format = procs_format, .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "procs:attr", .pattern = ":/cpu/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_parent_path_suffix = ":/cpu", .attributes = procs_items,
    .s_update_interval = UPDATE_INTERVAL_NEVER },
};

static gchar *procs_summarize_topology(int packs, int cores, int threads, int logical) {
    const gchar  *packs_fmt, *cores_fmt, *threads_fmt, *logical_fmt;
    gchar *ret = NULL, *full_fmt = NULL;
    packs_fmt = ngettext("%d physical processor", "%d physical processors", packs);
    cores_fmt = ngettext("%d core", "%d cores", cores);
    threads_fmt = ngettext("%d thread", "%d threads", threads);
    if (packs && cores && threads) {
        full_fmt = g_strdup_printf(_(/*/NP procs; NC cores; NT threads*/ "%s; %s; %s"), packs_fmt, cores_fmt, threads_fmt);
        ret = g_strdup_printf(full_fmt, packs, cores, threads);
    } else {
        logical_fmt = ngettext("%d logical processor", "%d  logical processors", logical);
        ret = g_strdup_printf(logical_fmt, logical);
    }
    g_free(full_fmt);
    return ret;
}

static gchar *summarize_children_by_counting_uniq_format_strings(sysobj *obj, int fmt_opts, const gchar *child_glob, gboolean nox) {
    gchar *ret = NULL;
    GSList *models = NULL, *l = NULL;
    GSList *childs = sysobj_children(obj, child_glob, NULL, FALSE);
    if (!childs) return NULL;
    for (l = childs; l; l = l->next) {
        models = g_slist_append(models, sysobj_format_from_fn(obj->path, (gchar*)l->data, fmt_opts | FMT_OPT_PART) );
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
                if (cur_count > 1 && !nox)
                    ret = appf(ret, " + ", "%dx %s", cur_count, cur_str);
                else
                    ret = appf(ret, " + ", "%s", cur_str);
                cur_str = model;
                cur_count = 1;
            } else
                cur_count++;
        }
    }
    if (cur_count > 1 && !nox)
        ret = appf(ret, " + ", "%dx %s", cur_count, cur_str);
    else
        ret = appf(ret, " + ", "%s", cur_str);
    g_slist_free_full(models, g_free);
    return ret;
}

static int count_children(const gchar *path, const gchar *child_glob) {
    sysobj *obj = sysobj_new_fast(path);
    GSList *childs = sysobj_children(obj, child_glob, NULL, FALSE);
    int n = childs ? g_slist_length(childs) : 0;
    g_slist_free_full(childs, g_free);
    sysobj_free(obj);
    return n;
}

static sysobj *first_child(const gchar *path, const gchar *child_glob) {
    sysobj *ret = NULL;
    sysobj *obj = sysobj_new_fast(path);
    GSList *childs = sysobj_children(obj, child_glob, NULL, FALSE);
    if (childs)
        ret = sysobj_new_from_fn(obj->path, childs->data);
    g_slist_free_full(childs, g_free);
    sysobj_free(obj);
    return ret;
}

static gchar *procs_format(sysobj *obj, int fmt_opts) {
    if (SEQ(obj->path, ":/cpu/topo_desc")) {
        int packs = 0, cores = 0, threads = 0, logical = 0;
        int mc = sscanf(obj->data.str, "%dxP %dxC %dxT", &packs, &cores, &threads);
        if (!threads)
            logical = count_children(":/cpu/cpuinfo", "logical_cpu*");
        return procs_summarize_topology(packs, cores, threads, logical);
    }
    if (SEQ(obj->path, ":/cpu")) {
        gchar *ret = NULL;
        int threads = sysobj_uint32_from_fn(obj->path, "threads", 10);
        gchar *tsum = sysobj_format_from_fn(obj->path, "topo_desc", fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);
        gchar *soc_vendor = sysobj_format_from_fn(obj->path, "soc_vendor", fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);
        gchar *soc_name = sysobj_format_from_fn(obj->path, "soc_name", fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);
        gchar *msum = summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "package*", FALSE);
        gchar *cisum = sysobj_format_from_fn(":/cpu/cpuinfo", NULL, fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);

        sysobj *log0 = first_child(":/cpu/cpuinfo", "logical_cpu*");
        gchar *funfacts = log0 ? sysobj_format_from_fn(log0->path, "x86_details", fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL) : NULL;
        sysobj_free(log0);

        if (fmt_opts & FMT_OPT_COMPLETE) {
            if (soc_vendor) {
                ret = appfsp(ret, "%s", soc_vendor);
                ret = appfsp(ret, "%s", soc_name);
            }
            if (msum)
                ret = appf(ret, "\n", "%s", msum);
            if (!threads && cisum)
                ret = appf(ret, "\n", "%s", cisum);
            if (funfacts)
                ret = appf(ret, "\n", "%s", funfacts);
            ret = appf(ret, "\n", "%s", tsum);
        } else {
            /* not FMT_OPT_COMPLETE */
            if (soc_vendor) {
                ret = appfsp(ret, "%s", soc_vendor);
                ret = appfsp(ret, "%s", soc_name);
            }
            else if (msum)
                ret = appfsp(ret, "%s", msum);
            else if (!threads && cisum)
                ret = appfsp(ret, "%s", cisum);

            if (!ret)
                ret = appfsp(ret, "%s", tsum);
        }

        g_free(soc_name);
        g_free(soc_vendor);
        g_free(funfacts);
        g_free(tsum);
        g_free(msum);
        g_free(cisum);
        return ret;
    }
    if (verify_lblnum(obj, "thread") ) {
        return sysobj_format_from_fn(obj->path, "cpuinfo", fmt_opts);
    }
    if (verify_lblnum(obj, "core") ) {
        return summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "thread*", TRUE);
    }
    if (verify_lblnum(obj, "package") ) {
        gboolean nox = TRUE;
        sysobj *log0 = first_child(":/cpu/cpuinfo", "logical_cpu*");
        if (log0 && log0->exists) {
            gchar *fam = sysobj_raw_from_fn(log0->path, "arch_family");
            if (SEQ(fam, "arm") ) nox = FALSE;
            g_free(fam);
        }
        sysobj_free(log0);
        return summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "core*", nox);
    }
    return simple_format(obj, fmt_opts);
}

void class_procs() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_procs); i++)
        class_add(&cls_procs[i]);
}

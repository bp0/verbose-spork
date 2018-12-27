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

static gchar *procs_format(sysobj *obj, int fmt_opts);
static vendor_list procs_vendors(sysobj *obj) { return sysobj_vendors_from_fn(obj->path, "cpuinfo"); }

static sysobj_class cls_procs[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "procs", .pattern = ":/cpu", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
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
                    ret = appfs(ret, " + ", "%dx %s", cur_count, cur_str);
                else
                    ret = appfs(ret, " + ", "%s", cur_str);
                cur_str = model;
                cur_count = 1;
            } else
                cur_count++;
        }
    }
    if (cur_count > 1 && !nox)
        ret = appfs(ret, " + ", "%dx %s", cur_count, cur_str);
    else
        ret = appfs(ret, " + ", "%s", cur_str);
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
    if (SEQ(obj->path, ":/cpu")) {
        gchar *ret = NULL;
        int packs = sysobj_uint32_from_fn(obj->path, "packs", 10);
        int cores = sysobj_uint32_from_fn(obj->path, "cores", 10);
        int threads = sysobj_uint32_from_fn(obj->path, "threads", 10);
        int logical = 0;
        if (!threads)
            logical = count_children(":/cpu/cpuinfo", "logical_cpu*");
        gchar *tsum = procs_summarize_topology(packs, cores, threads, logical);
        gchar *soc = sysobj_format_from_fn(obj->path, "soc_name", fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);
        gchar *msum = summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "package*", FALSE);
        gchar *cisum = sysobj_format_from_fn(":/cpu/cpuinfo", NULL, fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);

        if (fmt_opts & FMT_OPT_COMPLETE) {
            if (soc)
                ret = appfs(ret, "\n", "%s", soc);
            if (msum)
                ret = appfs(ret, "\n", "%s", msum);
            if (!threads && cisum)
                    ret = appfs(ret, "\n", "%s", cisum);
            ret = appfs(ret, "\n", "%s", tsum);
        } else {
            /* not FMT_OPT_COMPLETE */
            if (soc)
                ret = appf(ret, "%s", soc);
            else if (msum)
                ret = appf(ret, "%s", msum);
            else if (!threads && cisum)
                ret = appf(ret, "%s", cisum);

            if (!ret)
                ret = appf(ret, "%s", tsum);
        }

        g_free(soc);
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
        if (log0->exists) {
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

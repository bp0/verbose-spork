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
static double procs_update_interval(sysobj *obj);

static sysobj_class cls_procs[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "procs", .pattern = ":/procs*", .flags = OF_GLOB_PATTERN | OF_CONST,
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

static gchar *summarize_children_by_counting_uniq_format_strings(sysobj *obj, int fmt_opts, const gchar *child_glob) {
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
                if (cur_count > 1)
                    ret = appfs(ret, " + ", "%dx %s", cur_count, cur_str);
                else
                    ret = appfs(ret, " + ", "%s", cur_str);
                cur_str = model;
                cur_count = 1;
            } else
                cur_count++;
        }
    }
    if (cur_count > 1)
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

static gchar *procs_format(sysobj *obj, int fmt_opts) {
    if (SEQ(obj->name, "procs")) {
        gchar *ret = NULL;
        gchar *p_str = sysobj_raw_from_fn(obj->path, "packs");
        gchar *c_str = sysobj_raw_from_fn(obj->path, "cores");
        gchar *t_str = sysobj_raw_from_fn(obj->path, "threads");
        int packs = atoi(p_str);
        int cores = atoi(c_str);
        int threads = atoi(t_str);
        int logical = 0;
        if (!threads)
            logical = count_children(":/cpuinfo", "logical_cpu*");
        gchar *tsum = procs_summarize_topology(packs, cores, threads, logical);

        if (fmt_opts & FMT_OPT_COMPLETE) {
            gchar *soc = sysobj_format_from_fn(obj->path, "soc_name", fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);
            if (soc)
                ret = appfs(ret, "\n", "%s", soc);
            g_free(soc);

            gchar *msum = summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "package*");
            if (msum)
                ret = appfs(ret, "\n", "%s", msum);
            g_free(msum);

            if (!threads) {
                gchar *cisum = sysobj_format_from_fn(":/cpuinfo", NULL, fmt_opts | FMT_OPT_PART | FMT_OPT_OR_NULL);
                if (cisum)
                    ret = appfs(ret, "\n", "%s", cisum);
                g_free(cisum);
            }
        }

        ret = appfs(ret, "\n", "%s", tsum);

        g_free(tsum);
        g_free(p_str);
        g_free(c_str);
        g_free(t_str);
        return ret;
    }
    if (verify_lblnum(obj, "thread") ) {
        return sysobj_format_from_fn(obj->path, "cpuinfo", fmt_opts);
    }
    if (verify_lblnum(obj, "core") ) {
        return summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "thread*");
    }
    if (verify_lblnum(obj, "package") ) {
        return summarize_children_by_counting_uniq_format_strings(obj, fmt_opts, "core*");
    }
    return simple_format(obj, fmt_opts);
}

void class_procs() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_procs); i++)
        class_add(&cls_procs[i]);
}

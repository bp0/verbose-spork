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
  /* all else */
  { .tag = "procs", .pattern = ":/procs*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = procs_format, .f_update_interval = procs_update_interval },
};

static gchar *procs_summarize_topology(int packs, int cores, int threads) {
    const gchar  *packs_fmt, *cores_fmt, *threads_fmt;
    gchar *ret, *full_fmt;
    packs_fmt = ngettext("%d physical processor", "%d physical processors", packs);
    cores_fmt = ngettext("%d core", "%d cores", cores);
    threads_fmt = ngettext("%d thread", "%d threads", threads);
    full_fmt = g_strdup_printf(_(/*/NP procs; NC cores; NT threads*/ "%s; %s; %s"), packs_fmt, cores_fmt, threads_fmt);
    ret = g_strdup_printf(full_fmt, packs, cores, threads);
    g_free(full_fmt);
    return ret;
}

static gchar *procs_format(sysobj *obj, int fmt_opts) {
    if (!strcmp(obj->name, "procs")) {
        gchar *ret = NULL;
        gchar *p_str = sysobj_raw_from_fn(obj->path, "packs");
        gchar *c_str = sysobj_raw_from_fn(obj->path, "cores");
        gchar *t_str = sysobj_raw_from_fn(obj->path, "threads");
        int packs = atoi(p_str);
        int cores = atoi(c_str);
        int threads = atoi(t_str);
        ret = procs_summarize_topology(packs, cores, threads);
        g_free(p_str);
        g_free(c_str);
        g_free(t_str);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static double procs_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 0.0;
}

void class_procs() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_procs); i++) {
        class_add(&cls_procs[i]);
    }
}

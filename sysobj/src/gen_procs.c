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

/* Generator to create a tree of processors/cores/threads
 *  :/procs
 */
#include "sysobj.h"
#include "cpubits.h"

#define PROCS_ROOT ":/procs"

static sysobj_virt vol[] = {
    { .path = PROCS_ROOT, .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

gboolean cpu_x86_vfms(int logical, gchar **vendor_id, int *family, int *model, int *stepping) {
    gchar *cpuinfo_path = g_strdup_printf(":/cpuinfo/logical_cpu%d", logical);
    gchar *type_str =
        sysobj_raw_from_fn(cpuinfo_path, "_type");

    if (g_strcmp0(type_str, "x86") != 0) {
        g_free(type_str);
        return FALSE;
    }

    gchar *vendor_str =
        sysobj_raw_from_fn(cpuinfo_path, "vendor_id");
    gchar *family_str =
        sysobj_raw_from_fn(cpuinfo_path, "family");
    gchar *model_str =
        sysobj_raw_from_fn(cpuinfo_path, "model");
    gchar *stepping_str =
        sysobj_raw_from_fn(cpuinfo_path, "stepping");

    if (vendor_id)
        *vendor_id = vendor_str;
    else
        g_free(vendor_str);

    if (family_str && family) *family = atoi(family_str);
    if (model_str && model) *model = atoi(model_str);
    if (stepping_str && stepping) *stepping = atoi(stepping_str);

    g_free(type_str);
    g_free(family_str);
    g_free(model_str);
    g_free(stepping_str);
    return TRUE;
}

/* export */
/* obj points to a cpuN/topology */
void cpu_pct(sysobj *obj, int *logical, int *pack, int *core_of_pack, int *thread_of_core) {
    gchar *pn = sysobj_parent_name(obj);
    *logical = util_get_did(pn, "cpu");

    gchar *pack_str =
        sysobj_raw_from_fn(obj->path, "physical_package_id");
    gchar *core_id =
        sysobj_raw_from_fn(obj->path, "core_id");
    gchar *thread_sibs =
        sysobj_raw_from_fn(obj->path, "thread_siblings_list");

    *pack = pack_str ? strtol(pack_str, NULL, 10) : -1;
    *core_of_pack = core_id ? strtol(core_id, NULL, 10) : -1;
    *thread_of_core = -1;
    if (thread_sibs) {
        cpubits *bits = cpubits_from_str(thread_sibs);
        int sib = cpubits_min(bits);
        int i = 0;
        while (sib != *logical) {
            sib = cpubits_next(bits, sib, -1);
            i++;
        }
        if (sib == *logical)
            *thread_of_core = i;
        free(bits);
    }

    /* Old physical_package_id is always 0 even with multi-socket systems.
     * Just assume pack = logical if the processor is old enough. */
    gchar *x86_vid = NULL;
    int x86_f = 0, x86_m = 0;
    gboolean x86 = cpu_x86_vfms(*logical, &x86_vid, &x86_f, &x86_m, NULL);
    if (SEQ(x86_vid, "GenuineIntel") )
        if (x86_f < 6 || ( x86_f == 6 && (x86_m < 14 || x86_m == 21) ) )
            *pack = *logical;

    g_free(pack_str);
    g_free(core_id);
    g_free(thread_sibs);
    g_free(pn);
}

static void procs_scan() {
    static const char s_pack[] = "package";
    static const char s_core[] = "core";
    static const char s_thread[] = "thread";
    static const char s_clock[] = "freq_domain";

    int packs = 0, cores = 0, threads = 0, clocks = 0;
    GSList *uniq_clocks = NULL;

    sysobj_virt_remove(PROCS_ROOT "/*");

    sysobj *obj = sysobj_new_from_fn("/sys/devices/system/cpu", NULL);
    GSList *cpu_list = sysobj_children(obj, "cpu*", NULL, TRUE);
    GSList *l = cpu_list;
    while(l) {
        int log = 0, p = 0, c = 0, t = 0;
        sysobj *cpu_obj = sysobj_new_from_fn(obj->path, (gchar *)l->data);
        if (verify_lblnum(cpu_obj, "cpu")) {
            /* topo */
            sysobj *topo_obj = sysobj_new_from_fn(cpu_obj->path, "topology");
            cpu_pct(topo_obj, &log, &p, &c, &t);
            gchar *t_path = g_strdup_printf(PROCS_ROOT "/%s%d/%s%d/%s%d", s_pack, p, s_core, c, s_thread, t);
            gchar *c_path = g_strdup_printf(PROCS_ROOT "/%s%d/%s%d", s_pack, p, s_core, c);
            gchar *p_path = g_strdup_printf(PROCS_ROOT "/%s%d", s_pack, p);
            gchar *cpuinfo_path = g_strdup_printf(":/cpuinfo/logical_cpu%d", log);
            gchar *model = sysobj_raw_from_fn(cpuinfo_path, "model_name");

            /* sysobj_virt_add() returns TRUE only if added, so count the additions */
            if ( sysobj_virt_add_simple(p_path, NULL, "*", VSO_TYPE_DIR ) ) packs++;
            if ( sysobj_virt_add_simple(c_path, NULL, "*", VSO_TYPE_DIR ) ) cores++;
            if ( sysobj_virt_add_simple(t_path, NULL, "*", VSO_TYPE_DIR ) ) threads++;
            sysobj_virt_add_simple(t_path, cpu_obj->name, cpu_obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
            sysobj_virt_add_simple(t_path, "_cpuinfo", cpuinfo_path, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
            sysobj_virt_add_simple(p_path, "model_name", model, VSO_TYPE_STRING);

            g_free(t_path);
            g_free(c_path);
            g_free(p_path);
            g_free(cpuinfo_path);
            g_free(model);
            sysobj_free(topo_obj);

            /* clock */
            sysobj *freq_obj = sysobj_new_from_fn(cpu_obj->path, "cpufreq");
            gchar *freqdomain_cpus =
                sysobj_raw_from_fn(freq_obj->path, "freqdomain_cpus");
            if (!freqdomain_cpus)
                freqdomain_cpus = sysobj_raw_from_fn(freq_obj->path, "related_cpus");
            if (!freqdomain_cpus)
                freqdomain_cpus = sysobj_raw_from_fn(freq_obj->path, "affected_cpus");

            if (freqdomain_cpus) {
                g_strchomp(freqdomain_cpus); /* remove \n */

                int clk_id = 0;
                GSList *l = g_slist_find_custom(uniq_clocks, freqdomain_cpus, (GCompareFunc)g_strcmp0);
                if (l) {
                    clk_id = g_slist_position(uniq_clocks, l);
                } else {
                    clk_id = g_slist_length(uniq_clocks);
                    uniq_clocks = g_slist_append(uniq_clocks, g_strdup(freqdomain_cpus) );
                }

                gchar *clk_path = g_strdup_printf(PROCS_ROOT "/%s%d", s_clock, clk_id);
                if ( sysobj_virt_add_simple(clk_path, NULL, "*", VSO_TYPE_DIR ) ) clocks++;
                sysobj_virt_add_simple(clk_path, "cpu_list", freqdomain_cpus, VSO_TYPE_STRING);
                sysobj_virt_add_simple(clk_path, freq_obj->name, freq_obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
                g_free(clk_path);

                g_free(freqdomain_cpus);
            }
            sysobj_free(freq_obj);

        }
        sysobj_free(cpu_obj);
        g_free(l->data); /* won't need again */
        l = l->next;
    }
    g_slist_free(cpu_list);
    sysobj_free(obj);
    g_slist_free_full(uniq_clocks, (GDestroyNotify)g_free);

    sysobj_virt_add_simple(PROCS_ROOT "/packs", NULL, auto_free(g_strdup_printf("%d", packs) ), VSO_TYPE_STRING);
    sysobj_virt_add_simple(PROCS_ROOT "/cores", NULL, auto_free(g_strdup_printf("%d", cores) ), VSO_TYPE_STRING);
    sysobj_virt_add_simple(PROCS_ROOT "/threads", NULL, auto_free(g_strdup_printf("%d", threads) ), VSO_TYPE_STRING);
    sysobj_virt_add_simple(PROCS_ROOT "/freq_domains", NULL, auto_free(g_strdup_printf("%d", clocks) ), VSO_TYPE_STRING);
    free_auto_free();
}

void find_soc() {
    sysobj *obj = sysobj_new_from_fn("/sys/firmware/devicetree/base", "compatible");
    if (obj) {
        sysobj_read(obj, FALSE);
        /* compat string lists be like: item1\0item2\0item3\0 */
        if (obj->data.str) {
            gchar *el = obj->data.str;
            while(el < obj->data.str + obj->data.len) {
                gchar *lookup_path = g_strdup_printf(":/devicetree/dt.ids/%s", el);
                gchar *cls = sysobj_raw_from_fn(lookup_path, "class");
                if (SEQ(cls, "soc") ) {
                    gchar *vendor = sysobj_raw_from_fn(lookup_path, "vendor");
                    gchar *name = sysobj_raw_from_fn(lookup_path, "name");
                    if (!vendor) vendor = g_strdup("Unknown");
                    if (!name) name = g_strdup("Device");
                    sysobj_virt_add_simple(PROCS_ROOT "/soc_name", NULL, g_strdup_printf("%s %s", vendor, name), VSO_TYPE_STRING);
                    break;
                    g_free(vendor);
                    g_free(name);
                }
                g_free(lookup_path);
                el += strlen(el) + 1;
            }
        }
    }
    sysobj_free(obj);
}

void gen_procs() {
    int i = 0;
    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    procs_scan();
    find_soc();
}

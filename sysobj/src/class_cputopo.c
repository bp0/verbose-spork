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
#include "sysobj_extras.h" /* for cpu_pct() */
#include "cpubits.h"

gboolean cputopo_verify(sysobj *obj);
const gchar *cputopo_label(sysobj *obj);
gchar *cputopo_format(sysobj *obj, int fmt_opts);
guint cputopo_flags(sysobj *obj);

static sysobj_class cls_cputopo = { SYSOBJ_CLASS_DEF
    .tag = "cputopo", .pattern = "/sys/devices/system/cpu/cpu*/topology*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = cputopo_verify, .f_label = cputopo_label, .f_format = cputopo_format,
    .f_flags = cputopo_flags  };

static const struct { gchar *rp; gchar *lbl; int extra_flags; } cputopo_items[] = {
    { "topology",  N_("CPU Topology Information"), OF_NONE },
    { "physical_package_id", N_("Processor/Socket"), OF_NONE },
    { "core_id",             N_("CPU Core"), OF_NONE }, //TODO: of package?
    { NULL, NULL, 0 }
};

int cputopo_lookup(const gchar *key) {
    int i = 0;
    while(cputopo_items[i].rp) {
        if (strcmp(key, cputopo_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

gboolean cputopo_verify(sysobj *obj) {
    int i = cputopo_lookup(obj->name);
    if (i != -1)
        return TRUE;
    if (!strcmp(obj->name, "topology")) {
        if (verify_lblnum_child(obj, "cpu"))
            return TRUE;
    }
/*
     else if (verify_parent_name(obj, "topology"))
        return TRUE;
*/
    return FALSE;
}

const gchar *cputopo_label(sysobj *obj) {
    int i = cputopo_lookup(obj->name);
    if (i != -1)
        return _(cputopo_items[i].lbl);
    return NULL;
}

gchar *topology_summary(sysobj *obj, int fmt_opts) {
    static const char sfmt[] = N_("{P%d:C%d:T%d}");
    static const char lfmt[] = N_("{Processor %d : Core %d : Thread %d}");
    const char *fmt = (fmt_opts & FMT_OPT_SHORT) ? sfmt : lfmt;
    int logical = 0, pack = 0, core_of_pack = 0, thread_of_core = 0;
    cpu_pct(obj, &logical, &pack, &core_of_pack, &thread_of_core);
    return g_strdup_printf(
        (fmt_opts & FMT_OPT_NO_TRANSLATE) ? fmt : _(fmt),
        pack, core_of_pack, thread_of_core);
}

gchar *cputopo_format(sysobj *obj, int fmt_opts) {
    if (!strcmp(obj->name, "topology"))
        return topology_summary(obj, fmt_opts);
    return simple_format(obj, fmt_opts);
}

guint cputopo_flags(sysobj *obj) {
    if (obj) {
        int i = cputopo_lookup(obj->name);
        if (i != -1)
            return obj->cls->flags | cputopo_items[i].extra_flags;
    }
    return cls_cputopo.flags; /* remember to handle obj == NULL */
}

void class_cputopo() {
    class_add(&cls_cputopo);
}

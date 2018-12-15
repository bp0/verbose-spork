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
gchar *cputopo_format(sysobj *obj, int fmt_opts);

attr_tab cputopo_items[] = {
    { "topology",            N_("CPU topology information") },
    { "physical_package_id", N_("processor/socket") },
    { "core_id",             N_("CPU core") }, //TODO: of package?
    ATTR_TAB_LAST
};

static sysobj_class cls_cputopo = { SYSOBJ_CLASS_DEF
    .tag = "cputopo", .pattern = "/sys/devices/system/cpu/cpu*/topology*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = cputopo_items,
    .f_verify = cputopo_verify, .f_format = cputopo_format };

gboolean cputopo_verify(sysobj *obj) {
    int i = attr_tab_lookup(cputopo_items, obj->name);
    if (i != -1)
        return TRUE;
    if (SEQ(obj->name, "topology")) {
        if (verify_lblnum_child(obj, "cpu"))
            return TRUE;
    }
/*
     else if (verify_parent_name(obj, "topology"))
        return TRUE;
*/
    return FALSE;
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
    if (SEQ(obj->name, "topology"))
        return topology_summary(obj, fmt_opts);
    return simple_format(obj, fmt_opts);
}

void class_cputopo() {
    class_add(&cls_cputopo);
}

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

/*
 * - generators create virtual sysobj's
 *    - can't use sysobj_format, only sysobj_raw as there are no classes yet.
 * - classes provide interpretation and formatting of sysobj's
 */

void gen_sysobj();
void gen_dt();
void gen_pci_ids();
void gen_usb_ids();
void gen_dt_ids();
void gen_os_release();
void gen_dmidecode();
void gen_mobo(); /* requires :/dmidecode */
void gen_rpi();
void gen_cpuinfo();
void gen_meminfo();
void gen_procs(); /* requires :/cpuinfo */
void gen_gpu();   /* requires gen_*_ids, gen_dt */

void generators_init() {
    gen_sysobj(); /* internals, like vsysfs root (":") */

    gen_dt();
    gen_pci_ids();
    gen_usb_ids();
    gen_dt_ids();
    gen_os_release();
    gen_dmidecode();
    gen_mobo();
    gen_rpi();
    gen_cpuinfo();
    gen_meminfo();
    gen_procs();
    gen_gpu();
}

void class_power();
void class_os_release();
void class_mobo();
void class_rpi();
void class_cpuinfo();
void class_meminfo();
void class_procs();
void class_gpu();
void class_hwmon();
void class_devfreq();

void class_uptime();
void class_dmi_id();
void class_dt();
void class_cpu();
void class_cpufreq();
void class_cpucache();
void class_cputopo();
void class_pci();
void class_usb();
void class_os_release();
void class_proc_alts();
void class_any_utf8();

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
    if (flags & OF_IS_VENDOR)
        flags_list = appfs(flags_list, " | ", "%s", "OF_IS_VENDOR");

    if (flags_list) {
        gchar *ret = g_strdup_printf("[%s] %s", obj->data.str, flags_list);
        g_free(flags_list);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static attr_tab sysobj_items[] = {
    { "elapsed", N_("seconds since sysobj_init()"), OF_NONE, fmt_seconds_to_span, 0.2 },
    { "class_count", N_("number of sysobj classes defined"), OF_NONE, NULL, 0.5 },
    { "virt_count", N_("number of objects in the sysobj virtual fs tree"), OF_NONE, NULL, 0.5 },
    { "root", N_("the alternate filesystem root, if any") },
    { "free_expected", N_("seconds until next free_auto_free()"), OF_NONE, fmt_seconds, 0.2 },
    { "freed_count", N_("memory objects freed via auto_free()"), OF_NONE, NULL, 0.5 },
    { "free_queue", N_("memory objects waiting to be freed via auto_free()"), OF_NONE, NULL, 0.5 },
    { "sysobj_new", N_("sysobj created with sysobj_new()") },
    { "sysobj_new_fast", N_("sysobj created without sysobj_classify()") },
    { "sysobj_clean", N_("sysobj cleared") },
    { "sysobj_free", N_("sysobj freed") },
    ATTR_TAB_LAST
};

static sysobj_class cls_internal[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "vsfs", .pattern = ":", .flags = OF_CONST,
    .s_label = N_("Virtual sysfs root"), .s_update_interval = 60.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:stat", .pattern = ":sysobj/*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .attributes = sysobj_items, .s_update_interval = 0.5 },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:class", .pattern = ":sysobj/classes/*", .flags = OF_CONST | OF_GLOB_PATTERN,
    .f_verify = class_verify, .f_format = class_format },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:class:flags", .pattern = ":sysobj/classes/*/.flags", .flags = OF_CONST | OF_GLOB_PATTERN,
    .f_format = class_flags_format },
  { SYSOBJ_CLASS_DEF
    .tag = "sysobj:class:hits", .pattern = ":sysobj/classes/*/hits", .flags = OF_CONST | OF_GLOB_PATTERN,
    .s_update_interval = 0.5 },
};

void class_internal() {
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_internal); i++)
        class_add(&cls_internal[i]);
}

void class_init() {
    generators_init();
    class_internal();

    class_power();
    class_proc_alts();

    class_os_release();
    class_mobo();
    class_rpi();
    class_cpuinfo();
    class_meminfo();
    class_procs();
    class_gpu();
    class_hwmon();
    class_devfreq();

    class_cpu();
    class_cpufreq();
    class_cpucache();
    class_cputopo();
    class_pci();
    class_usb();
    class_uptime();
/* consumes every direct child, careful with order */
    class_dmi_id();
    class_dt();

/* anything left that is human-readable */
    class_any_utf8(); /* OF_BLAST */
}

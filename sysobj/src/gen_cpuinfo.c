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

/* Generator turns /proc/cpuinfo into a sysobj tree
 *  :/cpuinfo
 */

#include "sysobj.h"
#include "sysobj_extras.h"
#include "vendor.h"
#include "arm_data.h"
#include "x86_data.h"

#define PROC_CPUINFO "/proc/cpuinfo"

/* guess from what appears in the cpuinfo, because we
 * could be testing on a different arch. */
enum {
    CPU_TYPE_UNK = 0,
    CPU_TYPE_ARM = 1,
    CPU_TYPE_X86 = 2,
    CPU_TYPE_RV  = 3,
};
static const gchar *cpu_types[] = { "unknown", "arm", "x86", "risc-v" };

static int cpuinfo_was_found = 0;
static gchar *cpuinfo_found(const gchar *path) {
    PARAM_NOT_UNUSED(path);
    return g_strdup(cpuinfo_was_found ? "1" : "0");
}

static sysobj_virt vol[] = {
    { .path = ":/cpuinfo/_found", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = cpuinfo_found, .f_get_type = NULL },
    { .path = ":/cpuinfo", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

static GList *lcpus = NULL;
typedef struct {
    int id, type;
    gchar *model_name;
    GSList *flags; /* includes flags, bugs, pm, etc */
    /* arm */
    gchar *linux_name;
    gchar *cpu_implementer;
    gchar *cpu_architecture;
    gchar *cpu_variant;
    gchar *cpu_part;
    gchar *cpu_revision;
    /* x86 */
    gchar *model, *family, *stepping;
    gchar *cpuid_level;
    gchar *apicid, *apicid_initial;
    gchar *vendor_id;
    gchar *microcode;
    gchar *tlb_size;
    gchar *clflush_size;
    gchar *cache_alignment;
    gchar *address_sizes;
    gchar *cpu_mhz;
    gchar *cache_size;
    gchar *bogomips;

    const Vendor *vendor;
} lcpu;

void lcpu_free(lcpu *s) {
    if (!s) return;
    g_slist_free_full(s->flags, g_free);
    /* arm */
    g_free(s->linux_name);
    g_free(s->cpu_implementer);
    g_free(s->cpu_architecture);
    g_free(s->cpu_variant);
    g_free(s->cpu_part);
    g_free(s->cpu_revision);
    /* x86 */
    g_free(s->model_name);
    g_free(s->vendor_id);
    g_free(s->microcode);
    g_free(s->tlb_size);
    g_free(s->clflush_size);
    g_free(s->cache_alignment);
    g_free(s->address_sizes);
    g_free(s->model);
    g_free(s->family);
    g_free(s->stepping);
    g_free(s->cpuid_level);
    g_free(s->apicid);
    g_free(s->apicid_initial);
    g_free(s->cpu_mhz);
    g_free(s->cache_size);
    g_free(s->bogomips);
    /* self */
    g_free(s);
}

gboolean cpuinfo_arm_decoded_name(lcpu *c) {
    if ( c->cpu_architecture ) {
        char *dn = arm_decoded_name(c->cpu_implementer, c->cpu_part, c->cpu_variant, c->cpu_revision, c->cpu_architecture);
        if (dn) {
            c->linux_name = c->model_name;
            c->model_name = g_strdup(dn);
            free(dn);
        }
        return TRUE;
    }
    return FALSE;
}

static void cpuinfo_append_flags(lcpu *p, gchar *prefix, gchar *flags_str) {
    gchar **each = g_strsplit(flags_str, " ", -1);
    gsize i, count = g_strv_length(each);
    gchar *tmp = NULL;
    for (i = 0; i < count; i++) {
        if (prefix)
            tmp = g_strdup_printf("%s:%s", prefix, each[i]);
        else
            tmp = g_strdup(each[i]);
        p->flags = g_slist_append(p->flags, tmp);
    }
    g_strfreev(each);
}

/* Some old cpuinfo's for single-cpu systems don't
 * give a "proccessor: 0" a the beginning.
 * So, if any value is seen before this_cpu is defined,
 * then assume that was the case and create a processor 0. */
#define CHKONEPROC if (!this_lcpu) { this_lcpu = g_new0(lcpu, 1); /* id=0 */}
#define CHKFOR(p) (g_str_has_prefix(line, p))
#define CHKSETFOR_EZ(p) if (CHKFOR(#p)) { CHKONEPROC; this_lcpu->p = g_strdup(value); continue; }
#define CHKSETFOR(p, m) if (CHKFOR(p)) { CHKONEPROC; this_lcpu->m = g_strdup(value); continue; }
#define CHKSETFOR_INT(p, m) if (CHKFOR(p)) { CHKONEPROC; this_lcpu->m = atol(value); continue; }

/* because g_slist_copy_deep( .. , g_strdup, NULL)
 * would have been too easy */
static gchar *dumb_string_copy(gchar *src, gpointer *e) {
    PARAM_NOT_UNUSED(e);
    return g_strdup(src);
}

void cpuinfo_scan_arm_x86(gchar **lines, gsize line_count) {
    gchar rep_pname[256] = "";
    lcpu *this_lcpu = NULL;
    gsize i = 0;
    for (i = 0; i < line_count; i++) {
        gchar *line = lines[i];
        gchar *value = g_utf8_strchr(line, -1, ':');
        if (value)
            value = g_strstrip(value+1);
        else
            continue;

        if (CHKFOR("Processor")) { /* note the majiscule P */
            strcpy(rep_pname, value);
            continue;
        }

        if (CHKFOR("processor")) {
            /* finish previous */
            if (this_lcpu) {
                if (!this_lcpu->model_name)
                    this_lcpu->model_name = g_strdup(rep_pname);
                lcpus = g_list_append(lcpus, this_lcpu);
            }

            /* start next */
            this_lcpu = g_new0(lcpu, 1);
            this_lcpu->id = atol(value);
            continue;
        }

        /* arm */
        if (CHKFOR("Features")) {
            cpuinfo_append_flags(this_lcpu, NULL, value);
            continue;
        }
        CHKSETFOR("CPU implementer", cpu_implementer);
        CHKSETFOR("CPU architecture", cpu_architecture);
        CHKSETFOR("CPU variant", cpu_variant);
        CHKSETFOR("CPU part", cpu_part);
        CHKSETFOR("CPU revision", cpu_revision);

        /* x86 */
        if (CHKFOR("flags")) {
            cpuinfo_append_flags(this_lcpu, NULL, value);
            continue;
        }
        if (CHKFOR("bugs")) {
            cpuinfo_append_flags(this_lcpu, "bug", value);
            continue;
        }
        if (CHKFOR("power management")) {
            cpuinfo_append_flags(this_lcpu, "pm", value);
            continue;
        }
        CHKSETFOR("model name", model_name);
        CHKSETFOR_EZ(vendor_id);
        CHKSETFOR_EZ(microcode);
        CHKSETFOR_EZ(cache_alignment);
        CHKSETFOR("TLB size", tlb_size);
        CHKSETFOR("clflush size", clflush_size);
        CHKSETFOR("address sizes", address_sizes);
        CHKSETFOR("cpuid level", cpuid_level);
        CHKSETFOR("cpu family", family);
        CHKSETFOR_EZ(model);
        CHKSETFOR_EZ(stepping);
        CHKSETFOR_EZ(apicid);
        CHKSETFOR("initial apicid", apicid_initial);
        CHKSETFOR("cpu MHz", cpu_mhz);
        CHKSETFOR("cache size", cache_size);

        CHKSETFOR("bogomips", bogomips);
        CHKSETFOR("BogoMips", bogomips);
        CHKSETFOR("BogoMIPS", bogomips);

        //TODO: old cpu bugs, like f00f

    }
    /* finish last */
    if (this_lcpu) {
        if (!this_lcpu->model_name)
            this_lcpu->model_name = g_strdup(rep_pname);
        lcpus = g_list_append(lcpus, this_lcpu);
    }

    /* re-duplicate missing data for /proc/cpuinfo variant that de-duplicated it */
#define REDUP(f) if (dlcpu->f && !this_lcpu->f) this_lcpu->f = g_strdup(dlcpu->f);
    lcpu *dlcpu = NULL;
    GList *l = g_list_last(lcpus);
    while (l) {
        this_lcpu = l->data;
        if (this_lcpu->flags) {
            dlcpu = this_lcpu;
        } else if (dlcpu) {
            if (dlcpu->flags && !this_lcpu->flags) {
                this_lcpu->flags = g_slist_copy_deep(dlcpu->flags, (GCopyFunc)dumb_string_copy, NULL);
            }
            REDUP(cpu_implementer);
            REDUP(cpu_architecture);
            REDUP(cpu_variant);
            REDUP(cpu_part);
            REDUP(cpu_revision);
        }
        l = l->prev;
    }

    /* and one more pass */
    l = lcpus;
    while(l) {
        this_lcpu = l->data;

        /* guess what we've got */

        if (this_lcpu->stepping) {
            this_lcpu->type = CPU_TYPE_X86;
            if (this_lcpu->vendor_id)
                this_lcpu->vendor = vendor_match(this_lcpu->vendor_id, NULL);
        }

        if (this_lcpu->cpu_architecture) {
            this_lcpu->type = CPU_TYPE_ARM;
            if (*(this_lcpu->model_name) == 0) {
                g_free(this_lcpu->model_name);
                this_lcpu->model_name = g_strdup("ARM Processor");
            }
            cpuinfo_arm_decoded_name(this_lcpu);
        }

        /* compress model name:
         * "Intel(R) Pentium(R) III CPU - S         1400MHz" -> "Intel(R) Pentium(R) III CPU - S 1400MHz"
         */
        util_compress_space(this_lcpu->model_name);

        l = l->next;
    }

}

#define EASY_VOM(m) if( this_lcpu->m ) sysobj_virt_add_simple(base, #m, this_lcpu->m, VSO_TYPE_STRING );
#define EASY_VOM_INT(m) if( this_lcpu->m ) { \
        gchar *str##m = g_strdup_printf("%lld", (long long int)this_lcpu->m); \
        sysobj_virt_add_simple(base, #m, str##m, VSO_TYPE_STRING ); \
        g_free(str##m); }
void cpuinfo_scan() {
    sysobj *obj = sysobj_new_from_fn("/proc/cpuinfo", NULL);
    sysobj_read(obj, FALSE);
    gchar **cpuinfo = g_strsplit(obj->data.str, "\n", -1);
    gsize line_count = g_strv_length(cpuinfo);
    cpuinfo_scan_arm_x86(cpuinfo, line_count);
    g_strfreev(cpuinfo);

    GList *l = lcpus;
    while(l) {
        lcpu *this_lcpu = l->data;
        gchar *type = g_strdup(cpu_types[this_lcpu->type]);
        gchar *sysfs = g_strdup_printf("/sys/devices/system/cpu/cpu%d", this_lcpu->id);
        gchar *base = g_strdup_printf(":/cpuinfo/logical_cpu%d", this_lcpu->id);
        //printf("%s\n", base);
        sysobj_virt_add_simple(base, NULL, "*", VSO_TYPE_DIR);
        sysobj_virt_add_simple(base, "_sysfs", sysfs, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
        sysobj_virt_add_simple(base, "_type", type, VSO_TYPE_STRING );
        EASY_VOM(model_name);

        gchar *base_flags = g_strdup_printf("%s/%s", base, "flags");
        sysobj_virt_add_simple(base_flags, NULL, "*", VSO_TYPE_DIR);
        if (this_lcpu->flags) {
            GSList *fl = this_lcpu->flags;
            while(fl) {
                gchar *flag = fl->data;
                sysobj_virt_add_simple(base_flags, flag, flag, VSO_TYPE_STRING );
                fl = fl->next;
            }
        }
        g_free(base_flags);

        /* arm */
        EASY_VOM(linux_name);
        EASY_VOM(cpu_implementer);
        EASY_VOM(cpu_architecture);
        EASY_VOM(cpu_variant);
        EASY_VOM(cpu_part);
        EASY_VOM(cpu_revision);
        /* x86 */
        EASY_VOM(vendor_id);
        EASY_VOM(microcode);
        EASY_VOM(tlb_size);
        EASY_VOM(clflush_size);
        EASY_VOM(cache_alignment);
        EASY_VOM(address_sizes);
        EASY_VOM(family);
        EASY_VOM(model);
        EASY_VOM(stepping);
        EASY_VOM(cpuid_level);
        EASY_VOM(apicid_initial);
        EASY_VOM(apicid);
        EASY_VOM(cache_size);
        EASY_VOM(cpu_mhz);
        EASY_VOM(bogomips);

        if (this_lcpu->vendor)
            sysobj_virt_add_vendor_match(base, "vendor", this_lcpu->vendor);

        g_free(base);
        l = l->next;
    }
    g_list_free_full(lcpus, (GDestroyNotify)lcpu_free);
    sysobj_free(obj);
}

void gen_cpuinfo() {
    int i = 0;

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    if (sysobj_exists_from_fn(PROC_CPUINFO, NULL)) {
        cpuinfo_was_found = 1;
        cpuinfo_scan();
    }
}

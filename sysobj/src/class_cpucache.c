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

#include <stdio.h> /* for sscanf() */
#include "sysobj.h"
#include "format_funcs.h"

const gchar cpucache_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-system-cpu") "\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-devices-system-cpu") "\n"
    "\n";

gchar *cpucache_format_type(sysobj *obj, int fmt_opts) {
    static const struct {
        const char *nshort;
        const char *nlong;
        const char *gtkey; /* Not used directly, creates translatable strings */
    } cache_types[] = {
        { "D", "Data",        NC_("cache-type", /*/cache type, as appears in: Level 1 (Data)*/ "Data") },
        { "I", "Instruction", NC_("cache-type", /*/cache type, as appears in: Level 1 (Instruction)*/ "Instruction") },
        { "U", "Unified",     NC_("cache-type", /*/cache type, as appears in: Level 2 (Unified)*/ "Unified") },
        { NULL, NULL, NULL }
    };

    if (obj) {
        if (obj->data.str) {
            gchar *ret = NULL;
            gchar *nstr = g_strdup(obj->data.str);
            g_strchomp(nstr);

            if (fmt_opts & FMT_OPT_SHORT) {
                int i = 0;
                while(cache_types[i].nlong) {
                    if (strcmp(cache_types[i].nlong, nstr) == 0) {
                        ret = g_strdup( cache_types[i].nshort );
                        break;
                    }
                    i++;
                }
            }
            if (!ret)
                ret = g_strdup(C_("cache-type", nstr));
            g_free(nstr);
            return ret;
        }
    }
    return simple_format(obj, fmt_opts);
}

gchar *cpucache_format_size(sysobj *obj, int fmt_opts) {
    if (obj) {
        if (obj->data.str) {
            uint32_t szK = 0;
            if ( sscanf(obj->data.str, "%uK", &szK) )
                return g_strdup_printf("%u%s%s",
                    szK,
                    (fmt_opts & FMT_OPT_SHORT) ? "" : " ",
                    (fmt_opts & FMT_OPT_NO_UNIT) ? "" : _("KB") );
        }
    }
    return simple_format(obj, fmt_opts);
}

gchar *cpucache_format_index(sysobj *obj, int fmt_opts) {
    if (obj) {
        gchar *ret = NULL;
        /* get nice description */
        gchar *level =
            sysobj_format_from_fn(obj->path, "level", fmt_opts | FMT_OPT_PART);
        gchar *assoc =
            sysobj_format_from_fn(obj->path, "ways_of_associativity", fmt_opts | FMT_OPT_PART);
        gchar *type =
            sysobj_format_from_fn(obj->path, "type", fmt_opts | FMT_OPT_PART);
        gchar *sets =
            sysobj_format_from_fn(obj->path, "number_of_sets", fmt_opts | FMT_OPT_PART);
        gchar *size =
            sysobj_format_from_fn(obj->path, "size", fmt_opts | FMT_OPT_PART);

        if (fmt_opts & FMT_OPT_SHORT) {
            ret = g_strdup_printf("L%s%s:%s", level, type, size );
        } else {
            ret = g_strdup_printf("Level %s (%s); %s-way set-associative; %s sets; %s size",
                    level, type, assoc, sets, size );
        }

        g_free(level);
        g_free(assoc);
        g_free(type);
        g_free(sets);
        g_free(size);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

gchar *cpucache_format_collection(sysobj *obj, int fmt_opts) {
    if (obj) {
        gchar ret[256] = "";
        int count = 0;
        GSList *index_list = sysobj_children(obj, "index*", NULL, TRUE);
        GSList *l = index_list;
        while(l) {
            //TODO: check length
            gchar *ix = sysobj_format_from_fn(obj->path, l->data, fmt_opts | FMT_OPT_SHORT | FMT_OPT_PART );
            if (ix) {
                if (count)
                    strcat(ret, " ");
                strcat(ret, ix);
                count++;
            }
            g_free(ix);
            l = l->next;
        }
        return g_strdup(ret);
    }
    return simple_format(obj, fmt_opts);
}

static gchar *fmt_cpucache_write_policy(sysobj *obj, int fmt_opts) {
    static lookup_tab cpucache_write_policy[] = {
        { "WriteThrough", N_("data is written to both the cache line and to the block in the lower-level memory") },
        { "WriteBack", N_("data is written only to the cache line and the modified cache line is written to main memory only when it is replaced") },
        LOOKUP_TAB_LAST
    };
    gchar *ret = sysobj_format_lookup_tab(obj, cpucache_write_policy, fmt_opts);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static gchar *fmt_cpucache_allocation_policy(sysobj *obj, int fmt_opts) {
    static lookup_tab cpucache_allocation_policy[] = {
        { "WriteAllocate", N_("allocate on miss due to write") },
        { "ReadAllocate", N_("allocate on miss due to read") },
        { "ReadWriteAllocate", N_("allocate on miss due to read or write") },
        LOOKUP_TAB_LAST
    };
    gchar *ret = sysobj_format_lookup_tab(obj, cpucache_allocation_policy, fmt_opts);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static attr_tab cache_part_items[] = {
    { "size", N_("cache size"), OF_NONE, cpucache_format_size },
    { "type", N_("cache type"), OF_NONE, cpucache_format_type },
    { "ways_of_associativity", N_("degree of freedom in placing a particular block") },
    { "allocation_policy", NULL, OF_NONE, fmt_cpucache_allocation_policy },
    { "attributes" },
    { "coherency_line_size", N_("the minimum amount of data in bytes that gets transferred from memory to cache"), OF_NONE, fmt_bytes_to_higher },
    { "level", N_("the cache hierarchy in the multi-level cache") },
    { "number_of_sets", N_("total number of sets (of lines that share an index) in the cache") },
    { "physical_line_partition", N_("number of physical cache line per cache tag") },
    { "shared_cpu_list", N_("logical cpus sharing the cache") },
    { "shared_cpu_map" },
    { "write_policy", NULL, OF_NONE, fmt_cpucache_write_policy },
    { "id" },
    ATTR_TAB_LAST
};

static sysobj_class cls_cpucache[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "cache", .pattern = "/sys/*/cpu*/cache", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("CPU cache(s)"), .s_halp = cpucache_reference_markup_text,
    .v_lblnum_child = "cpu", .f_format = cpucache_format_collection },
  { SYSOBJ_CLASS_DEF
    .tag = "cache:part", .pattern = "/sys/*/cache/index*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("CPU cache"), .s_halp = cpucache_reference_markup_text,
    .v_lblnum = "index", .f_format = cpucache_format_index },
  { SYSOBJ_CLASS_DEF
    .tag = "cache:part:attr", .pattern = "/sys/*/cache/index*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum_child = "index", .attributes = cache_part_items,
    .s_halp = cpucache_reference_markup_text },
};

void class_cpucache() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_cpucache); i++)
        class_add(&cls_cpucache[i]);
}

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

#define VSPK_CLASS_TAG "cpu/cache"

/* in class_cpu.c */
gboolean cpu_verify_child(sysobj *obj);

gboolean cpucache_verify_index(sysobj *obj) {
    return verify_lblnum(obj, "index");
}

gboolean cpucache_verify_index_child(sysobj *obj) {
    return verify_lblnum_child(obj, "index");
}

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

void class_cpucache() {
    sysobj_class *c = NULL;

    c = g_new0(sysobj_class, 1);
    c->tag = VSPK_CLASS_TAG;
    c->pattern = "/sys/*/cpu*/cache";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpu_verify_child;
    c->f_format = cpucache_format_collection;
    c->s_label = _("CPU Cache(s)");
    c->s_info = NULL; //TODO:
    class_add(c);

    c = g_new0(sysobj_class, 1);
    c->tag = VSPK_CLASS_TAG;
    c->pattern = "/sys/*/cache/index*";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpucache_verify_index;
    c->f_format = cpucache_format_index;
    c->s_label = _("CPU Cache");
    c->s_info = NULL; //TODO:
    class_add(c);

    c = g_new0(sysobj_class, 1);
    c->tag = VSPK_CLASS_TAG;
    c->pattern = "/sys/*/cache/index*/type";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpucache_verify_index_child;
    c->f_format = cpucache_format_type;
    c->s_label = _("Cache Type");
    c->s_info = NULL; //TODO:
    class_add(c);

    c = g_new0(sysobj_class, 1);
    c->tag = VSPK_CLASS_TAG;
    c->pattern = "/sys/*/cache/index*/size";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpucache_verify_index_child;
    c->f_format = cpucache_format_size;
    c->s_label = _("Cache Size");
    c->s_info = NULL; //TODO:
    class_add(c);

}

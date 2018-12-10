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

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

static gchar *get_item_items[] = {
    "elapsed", "root", "class_count", "virt_count",
    "freed_count", "free_queue",
    "sysobj_new", "sysobj_new_fast",
    "sysobj_clean", "sysobj_free",
};

static gchar *get_item(const gchar *path) {
    if (!path) return NULL;
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (SEQ(name, "root") )
        return g_strdup_printf("%s", sysobj_root_get());
    if (SEQ(name, "elapsed") )
        return g_strdup_printf("%lf", sysobj_elapsed());
    if (SEQ(name, "class_count") )
        return g_strdup_printf("%lu", (long unsigned)class_count() );
    if (SEQ(name, "virt_count") )
        return g_strdup_printf("%lu", (long unsigned)sysobj_virt_count() );
    if (SEQ(name, "free_queue") )
        return g_strdup_printf("%llu", sysobj_stats.auto_free_len );
    if (SEQ(name, "freed_count") )
        return g_strdup_printf("%llu", sysobj_stats.auto_freed );
    if (SEQ(name, "sysobj_new") )
        return g_strdup_printf("%llu", sysobj_stats.so_new );
    if (SEQ(name, "sysobj_new_fast") )
        return g_strdup_printf("%llu", sysobj_stats.so_new_fast );
    if (SEQ(name, "sysobj_clean") )
        return g_strdup_printf("%llu", sysobj_stats.so_clean );
    if (SEQ(name, "sysobj_free") )
        return g_strdup_printf("%llu", sysobj_stats.so_free );

    return g_strdup("?");
}

static const gchar class_item_list[] =
  ".def_file\n"
  ".def_line\n"
  ".pattern\n"
  ".flags\n"
  ".s_label\n"
  ".s_halp\n"
  ".s_suggest\n"
  ".s_update_interval\n"
  ".f_verify\n"
  ".f_label\n"
  ".f_format\n"
  ".f_update_interval\n"
  ".f_compare\n"
  ".f_flags\n"
  ".f_cleanup\n"
  ".f_halp\n";

static gchar *get_class_info(const gchar *path) {
    if (!path) return NULL;
    gchar name[128] = "";
    buff_basename(path, name, 127);
    GSList *cl = class_get_list();
    GSList *l = NULL;

    const gchar cls_root[] = ":sysobj/classes";
    gsize crsz = strlen(cls_root);

    /* get list of class tags */
    if (SEQ(path, cls_root) ) {
        gchar *ret = NULL;
        for(l = cl; l; l = l->next) {
            sysobj_class *c = (sysobj_class *)l->data;
            ret = appfs(ret, "\n", "%s", c->tag);
        }
        return ret;
    }

    /* lookup */
    sysobj_class *match = NULL;
    gsize ml = 0;
    for(l = cl; l; l = l->next) {
        sysobj_class *c = (sysobj_class *)l->data;
        if (g_str_has_prefix(path + crsz + 1, c->tag) ) {
            if (strlen(c->tag) > ml) {
                match = c;
                ml = strlen(c->tag);
            }
        }
    }

    if (match) {
        if (SEQ(name, ".def_file") )
            return g_strdup(match->def_file);
        if (SEQ(name, ".def_line") ) {
            if (match->def_file)
                return g_strdup_printf("%d", match->def_line);
            else
                return NULL;
        }
        if (SEQ(name, ".pattern") )
            return g_strdup(match->pattern);
        if (SEQ(name, ".flags") )
            return g_strdup_printf("0x%lx", (long unsigned)match->flags);
        if (SEQ(name, ".s_label") )
            return g_strdup(match->s_label);
        if (SEQ(name, ".s_halp") )
            return g_strdup(match->s_halp);
        if (SEQ(name, ".s_suggest") )
            return g_strdup(match->s_suggest);
        if (SEQ(name, ".s_update_interval") )
            return g_strdup_printf("%0.4lf", match->s_update_interval);
        if (SEQ(name, ".f_verify") )
            return g_strdup_printf("%p", match->f_verify);
        if (SEQ(name, ".f_label") )
            return g_strdup_printf("%p", match->f_label);
        if (SEQ(name, ".f_format") )
            return g_strdup_printf("%p", match->f_format);
        if (SEQ(name, ".f_update_interval") )
            return g_strdup_printf("%p", match->f_update_interval);
        if (SEQ(name, ".f_compare") )
            return g_strdup_printf("%p", match->f_compare);
        if (SEQ(name, ".f_flags") )
            return g_strdup_printf("%p", match->f_flags);
        if (SEQ(name, ".f_cleanup") )
            return g_strdup_printf("%p", match->f_cleanup);
        if (SEQ(name, ".f_halp") )
            return g_strdup_printf("%p", match->f_halp);

        /* assume it is a class tag */
        return g_strdup(class_item_list);
    }

    return NULL;
}

static int get_class_info_type(const gchar *path) {
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (SEQ(path, ":sysobj/classes") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN;

    int ret = VSO_TYPE_DIR; //assume
    gchar **il = g_strsplit(class_item_list, "\n", -1);
    gsize l = g_strv_length(il);
    for(unsigned int i = 0; i < l; i++)
        if (SEQ(name, il[i]) ) {
            ret = VSO_TYPE_STRING;
            break;
        }
    g_strfreev(il);
    return ret;
}

static sysobj_virt vol[] = {
    /* the vsysfs  root */
    { .path = ":", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },

    /* handy /sys and /proc links */
    { .path = ":/sysfs", .str = "/sys",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":/procfs", .str = "/proc",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },

    /* internal stuff */
    { .path = ":sysobj", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":sysobj/classes", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = get_class_info, .f_get_type = get_class_info_type },
};

void gen_sysobj() {
    int i = 0;
    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    /* get_item()-able items */
    for (i = 0; i < (int)G_N_ELEMENTS(get_item_items); i++) {
        sysobj_virt *vo = sysobj_virt_new();
        vo->path = util_build_fn(":sysobj", get_item_items[i]);
        vo->type = VSO_TYPE_STRING,
        vo->f_get_data = get_item;
        sysobj_virt_add(vo);
    }

}

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

static gchar *get_item(const gchar *path) {
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (!g_strcmp0(name, "root") )
        return g_strdup_printf("%s", sysobj_root_get());
    if (!g_strcmp0(name, "elapsed") )
        return g_strdup_printf("%lf", sysobj_elapsed());
    if (!g_strcmp0(name, "class_count") )
        return g_strdup_printf("%lu", (long unsigned)class_count() );
    if (!g_strcmp0(name, "virt_count") )
        return g_strdup_printf("%lu", (long unsigned)sysobj_virt_count() );

    return g_strdup("?");
}

static const gchar class_item_list[] =
  "_def_file\n"
  "_def_line\n"
  "_pattern\n"
  "_flags\n"
  "_s_label\n"
  "_s_halp\n"
  "_s_suggest\n"
  "_s_update_interval\n"
  "_f_verify\n"
  "_f_label\n"
  "_f_format\n"
  "_f_update_interval\n"
  "_f_compare\n"
  "_f_flags\n"
  "_f_cleanup\n"
  "_f_halp\n";

static gchar *get_class_info(const gchar *path) {
    gchar name[128] = "";
    buff_basename(path, name, 127);
    GSList *cl = class_get_list();
    GSList *l = NULL;

    const gchar cls_root[] = ":sysobj/classes";
    gsize crsz = strlen(cls_root);

    /* get list of class tags */
    if (!strcmp(path, cls_root) ) {
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
        if (!strcmp(name, "_def_file") )
            return g_strdup(match->def_file);
        if (!strcmp(name, "_def_line") )
            if (match->def_file)
                return g_strdup_printf("%d", match->def_line);
            else
                return NULL;
        if (!strcmp(name, "_pattern") )
            return g_strdup(match->pattern);
        if (!strcmp(name, "_flags") )
            return g_strdup_printf("0x%lx", (long unsigned)match->flags);
        if (!strcmp(name, "_s_label") )
            return g_strdup(match->s_label);
        if (!strcmp(name, "_s_halp") )
            return g_strdup(match->s_halp);
        if (!strcmp(name, "_s_suggest") )
            return g_strdup(match->s_suggest);
        if (!strcmp(name, "_s_update_interval") )
            return g_strdup_printf("%0.4lf", match->s_update_interval);
        if (!strcmp(name, "_f_verify") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_verify);
        if (!strcmp(name, "_f_label") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_label);
        if (!strcmp(name, "_f_format") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_format);
        if (!strcmp(name, "_f_update_interval") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_update_interval);
        if (!strcmp(name, "_f_compare") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_compare);
        if (!strcmp(name, "_f_flags") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_flags);
        if (!strcmp(name, "_f_cleanup") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_cleanup);
        if (!strcmp(name, "_f_halp") )
            return g_strdup_printf("0x%llx", (long long unsigned)match->f_halp);

        /* assume it is a class tag */
        return g_strdup(class_item_list);
    }

    return NULL;
}

static int get_class_info_type(const gchar *path) {
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (!strcmp(path, ":sysobj/classes") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN;

    int ret = VSO_TYPE_DIR; //assume
    gchar **il = g_strsplit(class_item_list, "\n", -1);
    gsize l = g_strv_length(il);
    for(unsigned int i = 0; i < l; i++)
        if (!strcmp(name, il[i]) ) {
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
    { .path = ":sysobj/elapsed", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = get_item, .f_get_type = NULL },
    { .path = ":sysobj/root", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = get_item, .f_get_type = NULL },
    { .path = ":sysobj/class_count", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = get_item, .f_get_type = NULL },
    { .path = ":sysobj/virt_count", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = get_item, .f_get_type = NULL },
    { .path = ":sysobj/classes", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = get_class_info, .f_get_type = get_class_info_type },
};

void gen_sysobj() {
    int i = 0;
    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }
}

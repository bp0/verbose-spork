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
#include "gg_file.h"
#include "format_funcs.h"

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

static gchar *get_item_items[] = {
    "elapsed", "root",
    "freed_count", "free_queue", "free_expected",
    "free_delay",
    "sysobj_new", "sysobj_new_fast",
    "sysobj_clean", "sysobj_free",
    "sysobj_read_first", "sysobj_read_force",
    "sysobj_read_expired", "sysobj_read_not_expired",
    "sysobj_read_wo", "sysobj_read_bytes",
    "gg_file_total_wait",
    "virt_count", "virt_iter", "virt_rm",
    "virt_add", "virt_replace",
    "virt_fget", "virt_fset",
    "virt_fget_bytes", "virt_fset_bytes",
    "vo_list_count", "vo_tree_count",
    "class_count", "class_iter", "classify_none",
    "classify_pattern_cmp",
    "ven_iter",
    "filter_iter", "filter_pattern_cmp",
};

static gchar *get_item(const gchar *path) {
    if (!path) return NULL;
    gchar name[128] = "";
    buff_basename(path, name, 127);

    if (SEQ(name, "root") )
        return g_strdup_printf("%s", sysobj_root_get());

    if (SEQ(name, "free_queue") )
        return g_strdup_printf("%llu", sysobj_stats.auto_free_len );
    if (SEQ(name, "freed_count") )
        return g_strdup_printf("%llu", sysobj_stats.auto_freed );
    if (SEQ(name, "free_delay") )
        return g_strdup_printf("%u", (unsigned)AF_DELAY_SECONDS );

    if (SEQ(name, "sysobj_new") )
        return g_strdup_printf("%llu", sysobj_stats.so_new );
    if (SEQ(name, "sysobj_new_fast") )
        return g_strdup_printf("%llu", sysobj_stats.so_new_fast );
    if (SEQ(name, "sysobj_clean") )
        return g_strdup_printf("%llu", sysobj_stats.so_clean );
    if (SEQ(name, "sysobj_free") )
        return g_strdup_printf("%llu", sysobj_stats.so_free );

    if (SEQ(name, "sysobj_read_first") )
        return g_strdup_printf("%llu", sysobj_stats.so_read_first );
    if (SEQ(name, "sysobj_read_force") )
        return g_strdup_printf("%llu", sysobj_stats.so_read_force );
    if (SEQ(name, "sysobj_read_expired") )
        return g_strdup_printf("%llu", sysobj_stats.so_read_expired );
    if (SEQ(name, "sysobj_read_not_expired") )
        return g_strdup_printf("%llu", sysobj_stats.so_read_not_expired );
    if (SEQ(name, "sysobj_read_wo") )
        return g_strdup_printf("%llu", sysobj_stats.so_read_wo );
    if (SEQ(name, "sysobj_read_bytes") )
        return g_strdup_printf("%llu", sysobj_stats.so_read_bytes );

    if (SEQ(name, "gg_file_total_wait") )
        return g_strdup_printf("%llu", gg_file_get_total_wait() );

    if (SEQ(name, "virt_add") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_add );
    if (SEQ(name, "virt_replace") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_replace );
    if (SEQ(name, "virt_fget") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_getf );
    if (SEQ(name, "virt_fset") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_setf );
    if (SEQ(name, "virt_fget_bytes") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_getf_bytes );
    if (SEQ(name, "virt_fset_bytes") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_setf_bytes );
    if (SEQ(name, "virt_count") )
        return g_strdup_printf("%lu", (long unsigned)sysobj_virt_count() );
    if (SEQ(name, "vo_tree_count") )
        return g_strdup_printf("%lu", (long unsigned)sysobj_virt_count_ex(1) );
    if (SEQ(name, "vo_list_count") )
        return g_strdup_printf("%lu", (long unsigned)sysobj_virt_count_ex(2) );
    if (SEQ(name, "virt_iter") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_iter );
    if (SEQ(name, "virt_rm") )
        return g_strdup_printf("%llu", sysobj_stats.so_virt_rm );

    if (SEQ(name, "class_count") )
        return g_strdup_printf("%lu", (long unsigned)class_count() );
    if (SEQ(name, "class_iter") )
        return g_strdup_printf("%llu", sysobj_stats.so_class_iter );
    if (SEQ(name, "classify_none") )
        return g_strdup_printf("%llu", sysobj_stats.classify_none );
    if (SEQ(name, "classify_pattern_cmp") )
        return g_strdup_printf("%llu", sysobj_stats.classify_pattern_cmp );

    if (SEQ(name, "filter_iter") )
        return g_strdup_printf("%llu", sysobj_stats.so_filter_list_iter );
    if (SEQ(name, "filter_pattern_cmp") )
        return g_strdup_printf("%llu", sysobj_stats.so_filter_pattern_cmp );

    if (SEQ(name, "ven_iter") )
        return g_strdup_printf("%llu", sysobj_stats.ven_iter );

    double elapsed = sysobj_elapsed();
    if (SEQ(name, "elapsed") )
        return g_strdup_printf("%lf", elapsed);
    if (SEQ(name, "free_expected") )
        return g_strdup_printf("%lf", sysobj_stats.auto_free_next - elapsed);

    return g_strdup("?");
}

static const gchar class_item_list[] =
  ".attributes\n"
  ".def_file\n"
  ".def_line\n"
  ".pattern\n"
  ".flags\n"
  ".s_label\n"
  ".s_halp\n"
  ".s_suggest\n"
  ".s_update_interval\n"
  ".s_node_format\n"
  ".s_vendors_from_child\n"
  ".f_verify\n"
  ".f_label\n"
  ".f_format\n"
  ".f_update_interval\n"
  ".f_compare\n"
  ".f_flags\n"
  ".f_cleanup\n"
  ".f_halp\n"
  ".f_vendors\n"
  ".v_name\n"
  ".v_subsystem\n"
  ".v_subsystem_parent\n"
  ".v_lblnum\n"
  ".v_lblnum_child\n"
  ".v_parent_class\n"
  ".v_parent_path_suffix\n"
  ".v_is_node\n"
  ".v_is_attr\n"
  "self\n"
  "hits\n";

static const gchar attr_item_list[] =
    ".attr_name\n"
    ".s_label\n"
    ".extra_flags\n"
    ".fmt_func\n"
    ".s_update_interval\n";

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
            ret = appf(ret, "\n", "%s", c->tag);
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
        const gchar *after_match = path + crsz + 1 + ml;

        if (SEQ(name, "self") )
            return g_strdup_printf("%p", match);
        if (SEQ(name, ".attributes") )
            return g_strdup_printf("%p", match->attributes);
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
        if (SEQ(name, ".s_node_format") )
            return g_strdup(match->s_node_format);
        if (SEQ(name, ".s_update_interval") )
            return g_strdup_printf("%0.4lf", match->s_update_interval);
        if (SEQ(name, ".s_vendors_from_child") )
            return g_strdup(match->s_vendors_from_child);
        if (SEQ(name, ".f_verify") )
            return g_strdup_printf("%p", match->f_verify);
        if (SEQ(name, ".f_label") )
            return g_strdup_printf("%p", match->f_label);
        if (SEQ(name, ".f_format") ) {
            const gchar *fname = format_funcs_lookup(match->f_format);
            if (fname)
                return g_strdup_printf("%p %s", match->f_format, fname);
            return g_strdup_printf("%p", match->f_format);
        }
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
        if (SEQ(name, ".f_vendors") )
            return g_strdup_printf("%p", match->f_vendors);
        if (SEQ(name, ".v_name") )
            return g_strdup(match->v_name);
        if (SEQ(name, ".v_subsystem") )
            return g_strdup(match->v_subsystem);
        if (SEQ(name, ".v_subsystem_parent") )
            return g_strdup(match->v_subsystem_parent);
        if (SEQ(name, ".v_lblnum") )
            return g_strdup(match->v_lblnum);
        if (SEQ(name, ".v_lblnum_child") )
            return g_strdup(match->v_lblnum_child);
        if (SEQ(name, ".v_parent_path_suffix") )
            return g_strdup(match->v_parent_path_suffix);
        if (SEQ(name, ".v_parent_class") )
            return g_strdup(match->v_parent_class);
        if (SEQ(name, ".v_is_node") )
            return g_strdup(match->v_is_node ? "shall be" : "may be");
        if (SEQ(name, ".v_is_attr") )
            return g_strdup(match->v_is_attr ? "shall be" : "may be");
        if (SEQ(name, "hits") )
            return g_strdup_printf("%llu", match->hits);

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

    /*
    if (g_str_has_suffix(path, "/.attributes") )
        return VSO_TYPE_DIR;
    */

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
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },

    /* requires external utilities that don't use sysobj */
    { .path = ":/extern", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },

    /* value lookup paths, caches */
    { .path = ":/lookup", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },

    /* handy /sys and /proc links */
    { .path = ":/sysfs", .str = "/sys",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK | VSO_TYPE_CONST },
    { .path = ":/procfs", .str = "/proc",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK | VSO_TYPE_CONST },

    /* internal stuff */
    { .path = ":sysobj", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
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

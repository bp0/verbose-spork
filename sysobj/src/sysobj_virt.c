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
#include "sysobj_virt.h"

static GSList *vo_list = NULL;
static GMutex vo_list_lock;

int sysobj_virt_count() {
    return g_slist_length(vo_list);
}

static int virt_path_cmp(sysobj_virt *a, sysobj_virt *b) {
    /* one or both are null */
    if (!a) return b ? -1 : 0;
    if (!b) return 1;
    return g_strcmp0(a->path, b->path);
}

void sysobj_virt_remove(gchar *glob) {
    g_mutex_lock(&vo_list_lock);
    sysobj_filter *f = sysobj_filter_new(SO_FILTER_INCLUDE_IIF, glob);
    GSList *torm = sysobj_virt_all_paths();
    GSList *fl = g_slist_append(NULL, f);
    torm = sysobj_filter_list(torm, fl);
    GSList *l = torm, *t = NULL;
    while(l) {
        sysobj_virt svo = { .path = (gchar *)l->data };
        t = g_slist_find_custom(vo_list, &svo, (GCompareFunc)virt_path_cmp);
        if (t) {
            sysobj_virt *tv = t->data;
            //printf("rm virtual object %s\n", tv->path);
            sysobj_virt_free(tv);
            vo_list = g_slist_delete_link(vo_list, t);
        }
        g_free(l->data); /* won't need again */
        l = l->next;
    }
    g_slist_free(torm);
    g_slist_free_full(fl, (GDestroyNotify)sysobj_filter_free);
    g_mutex_unlock(&vo_list_lock);
}

gboolean sysobj_virt_add(sysobj_virt *vo) {
    if (vo) {
        g_mutex_lock(&vo_list_lock);
        GSList *l = vo_list;
        while(l) {
            sysobj_virt *lv = l->data;
            if (g_strcmp0(lv->path, vo->path) == 0) {
                /* already exists, overwrite */
                sysobj_virt_free(lv);
                l->data = (gpointer*)vo;
                g_mutex_unlock(&vo_list_lock);
                return FALSE;
            }
            l = l->next;
        }
        //printf("add virtual object: %s [%s]\n", vo->path, vo->str);
        vo_list = g_slist_append(vo_list, vo);
        g_mutex_unlock(&vo_list_lock);
        return TRUE;
    }
    return FALSE;
}

gboolean sysobj_virt_add_simple_mkpath(const gchar *base, const gchar *name, const gchar *data, int type) {
    gchar *tp = NULL, *tpp = NULL;
    tp = util_build_fn(base, name);
    tpp = g_path_get_dirname(tp);
    if (strlen(tpp) > 1) {
        if (!sysobj_virt_find(tpp) )
            sysobj_virt_add_simple_mkpath(tpp, NULL, "*", VSO_TYPE_DIR);
    }
    g_free(tp);
    g_free(tpp);
    return sysobj_virt_add_simple(base, name, data, type);
}

gboolean sysobj_virt_add_simple(const gchar *base, const gchar *name, const gchar *data, int type) {
    sysobj_virt *vo = g_new0(sysobj_virt, 1);
    vo->path = util_build_fn(base, name);
    vo->type = type;
    vo->str = g_strdup(data);
    return sysobj_virt_add(vo);
}

void sysobj_virt_from_lines(const gchar *base, const gchar *data_in, gboolean safe_names) {
    gchar **lines = g_strsplit(data_in, "\n", -1);
    for(int i = 0; lines[i]; i++) {
        gchar *c = g_utf8_strchr(lines[i], strlen(lines[i]), ':');
        if (c) {
            *c = 0;
            g_strchomp(lines[i]);
            gchar *key = (safe_names) ? util_safe_name(lines[i]) : g_strdup(lines[i]);
            gchar *value = g_strdup(g_strstrip(c + 1));
            sysobj_virt *vo = sysobj_virt_new();
            vo->path = util_build_fn(base, key);
            vo->str = value;
            sysobj_virt_add(vo);
            g_free(key);
        }
    }
    g_strfreev(lines);
}

void sysobj_virt_from_kv(const gchar *base, const gchar *kv_data_in) {
    const gchar before_first_group[] = "THE_NO-GROUP________GRUOP";
    GKeyFile *key_file = NULL;
    gchar **groups = NULL, **keys = NULL;
    int i = 0, j = 0;

    gchar *kv_data = g_strdup_printf("[%s]\n%s", before_first_group, kv_data_in);

    key_file = g_key_file_new();
    g_key_file_load_from_data(key_file, kv_data, strlen(kv_data), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);
    for (i = 0; groups[i]; i++) {
        gboolean is_bs_group = (SEQ(groups[i], before_first_group)) ? TRUE : FALSE;

        keys = g_key_file_get_keys(key_file, groups[i], NULL, NULL);

        if (!is_bs_group) {
            sysobj_virt *vo = g_new0(sysobj_virt, 1);
            vo->type = VSO_TYPE_DIR;
            vo->path = g_strdup_printf("%s/%s", base, groups[i]);
            vo->str = g_strdup("*");
            sysobj_virt_add(vo);
        }

        for (j = 0; keys[j]; j++) {
            sysobj_virt *vo = g_new0(sysobj_virt, 1);
            vo->type = VSO_TYPE_STRING;
            if (is_bs_group)
                vo->path = g_strdup_printf("%s/%s", base, keys[j]);
            else
                vo->path = g_strdup_printf("%s/%s/%s", base, groups[i], keys[j]);
            vo->str = g_key_file_get_value(key_file, groups[i], keys[j], NULL);
            sysobj_virt_add(vo);
        }
        g_strfreev(keys);
    }
    g_strfreev(groups);
    g_free(kv_data);
}

sysobj_virt *sysobj_virt_find(const gchar *path) {
    //DEBUG("find path %s", path);
    sysobj_virt *ret = NULL;
    gchar *spath = g_strdup(path);
    util_null_trailing_slash(spath);
    int best_len = 0;
    /* exact static match wins over longest dynamic match */
    for (GSList *l = vo_list; l; l = l->next) {
        sysobj_virt *vo = l->data;
        if (vo->type & VSO_TYPE_DYN && g_str_has_prefix(path, vo->path) ) {
            int len = strlen(vo->path);
            if (len > best_len) {
                ret = vo;
                best_len = len;
            }
        }
        if (SEQ(vo->path, spath)) {
            ret = vo;
            break;
        }
    }
    g_free(spath);
    //DEBUG("... %s", (ret) ? ret->path : "(NOT FOUND)");
    return ret;
}

/* returns the link or null if not a link */
gchar *sysobj_virt_is_symlink(gchar *req) {
    const sysobj_virt *vo = sysobj_virt_find(req);
    if (vo) {
        long long unsigned int t = sysobj_virt_get_type(vo, req);
        if (t & VSO_TYPE_SYMLINK)
            return sysobj_virt_get_data(vo, req);
    }
    return NULL;
}

gchar *sysobj_virt_symlink_entry(const sysobj_virt *vo, const gchar *target, const gchar *req) {
    gchar *ret = NULL;
    if (vo) {
        if (target && req) {
            const gchar *extry = req + strlen(vo->path);
            ret = g_strdup_printf("%s%s%s", target, (*extry == '/') ? "" : "/", extry);
            util_null_trailing_slash(ret);
            //DEBUG("---\ntarget=%s\nreq=%s\nextry=%s\nret=%s", target, req, extry, ret);
        }
    }
    return ret;
}

gchar *sysobj_virt_get_data(const sysobj_virt *vo, const gchar *req) {
    gchar *ret = NULL;
    if (vo) {
        if (vo->f_get_data)
            ret = vo->f_get_data(req ? req : vo->path);
        else
            ret = g_strdup(vo->str);

        if (ret && req &&
            vo->type & VSO_TYPE_AUTOLINK ) {
            gchar *res = sysobj_virt_symlink_entry(vo, ret, req);
            g_free(ret);
            ret = res;
        }
    }
    return ret;
}

int sysobj_virt_get_type(const sysobj_virt *vo, const gchar *req) {
    int ret = 0;
    if (vo) {
        if (vo->f_get_type)
            ret = vo->f_get_type(req ? req : vo->path);
        else
            ret = vo->type;
    }
    return ret;
}

GSList *sysobj_virt_all_paths() {
    GSList *ret = NULL;
    GSList *l = vo_list;
    while (l) {
        sysobj_virt *vo = l->data;
        ret = g_slist_append(ret, g_strdup(vo->path));
        l = l->next;
    }
    return ret;
}

static GSList *sysobj_virt_children_auto(const sysobj_virt *vo, const gchar *req) {
    GSList *ret = NULL;
    if (vo && req) {
        gchar *fn = NULL;
        gchar *spath = g_strdup_printf("%s/", req);
        gsize spl = strlen(spath);
        GSList *l = vo_list;
        while (l) {
            sysobj_virt *tvo = l->data;
            /* find all vo paths that are immediate children */
            if ( g_str_has_prefix(tvo->path, spath) ) {
                if (!strchr(tvo->path + spl, '/')) {
                    fn = g_path_get_basename(tvo->path);
                    ret = g_slist_append(ret, fn);
                }
            }

            l = l->next;
        }
        g_free(spath);
    }
    return ret;
}

GSList *sysobj_virt_children(const sysobj_virt *vo, const gchar *req) {
    GSList *ret = NULL;
    gchar **childs = NULL;
    int type = sysobj_virt_get_type(vo, req);
    if (type & VSO_TYPE_DIR) {
        int i = 0, len = 0;
        gchar *data = sysobj_virt_get_data(vo, req);
        if (!data || *data == '*') {
            return sysobj_virt_children_auto(vo, req);
        } else {
            childs = g_strsplit(data, "\n", 0);
            len = g_strv_length(childs);
            for(i = 0; i < len; i++) {
                if (*childs[i] != 0)
                    ret = g_slist_append(ret, g_strdup(childs[i]));
            }
            g_free(data);
            g_strfreev(childs);
        }
    }
    return ret;
}

void sysobj_virt_free(sysobj_virt *s) {
    if (!s) return;
    /* allow objects to cleanup */
    if (s->f_get_data && s->type & VSO_TYPE_CLEANUP) {
        gchar *trash = s->f_get_data(NULL);
        g_free(trash);
    }
    if (!(s->type & VSO_TYPE_CONST)) {
        g_free(s->path);
        g_free(s->str);
        g_free(s);
    }
}

void sysobj_virt_cleanup() {
    g_slist_free_full(vo_list, (GDestroyNotify)sysobj_virt_free);
}

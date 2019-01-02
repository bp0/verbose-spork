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

#define virt_msg(fmt, ...) fprintf (stderr, "[%s] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

static GTree *vo_tree = NULL;
static GSList *vo_list = NULL;
static GMutex vo_lock;

static gint g_strcmp0_data(const gchar *s1, const gchar *s2, gpointer np) { return g_strcmp0(s1,s2); }

void sysobj_virt_init() {
    vo_tree = g_tree_new_full((GCompareDataFunc)g_strcmp0_data, NULL, g_free, (GDestroyNotify)sysobj_virt_free);
}

/* 0 = both, 1 = vo_tree, 2 = vo_list */
int sysobj_virt_count_ex(int what) {
    switch(what) {
        case 2: return g_slist_length(vo_list);
        case 1: return g_tree_nnodes(vo_tree);
    }
    return g_tree_nnodes(vo_tree) + g_slist_length(vo_list);
}

static int virt_path_cmp(sysobj_virt *a, sysobj_virt *b) {
    /* one or both are null */
    if (!a) return b ? -1 : 0;
    if (!b) return 1;
    return g_strcmp0(a->path, b->path);
}

void _remove1(gchar *path) {
    //virt_msg("rm virtual %s", path);

    /* tree */
    gboolean found = FALSE;
    found = g_tree_remove(vo_tree, path);
    if (found) {
        sysobj_stats.so_virt_rm++;
        return;
    }

    /* list */
    sysobj_virt svo = { .path = path };
    GSList *t = g_slist_find_custom(vo_list, &svo, (GCompareFunc)virt_path_cmp);
    if (t) {
        sysobj_virt *tv = t->data;
        sysobj_virt_free(tv);
        vo_list = g_slist_delete_link(vo_list, t);
        sysobj_stats.so_virt_rm++;
    }
}

void sysobj_virt_remove(gchar *glob) {
    g_mutex_lock(&vo_lock);
    sysobj_filter *f = sysobj_filter_new(SO_FILTER_INCLUDE_IIF, glob);
    GSList *torm = sysobj_virt_all_paths();
    GSList *fl = g_slist_append(NULL, f);
    torm = sysobj_filter_list(torm, fl);
    GSList *l = torm;
    while(l) {
        _remove1(l->data);
        g_free(l->data); /* won't need again */
        l = l->next;
    }
    g_slist_free(torm);
    g_slist_free_full(fl, (GDestroyNotify)sysobj_filter_free);
    g_mutex_unlock(&vo_lock);
}

gboolean sysobj_virt_add(sysobj_virt *vo) {
    if (vo) {
        /* check for errors */
        if (vo->type == VSO_TYPE_CLEANUP
            && !vo->f_get_data) {
            virt_msg("requested cleanup but no .f_get_data for %s", vo->path);
            sysobj_virt_free(vo);
            return FALSE;
        }
        if (vo->type == VSO_TYPE_NONE
            && !vo->f_get_type) {
            virt_msg("no .type or .f_get_type for %s", vo->path);
            sysobj_virt_free(vo);
            return FALSE;
        }
        if (!vo->f_get_data
            && !(vo->type & VSO_TYPE_SYMLINK)
            && vo->type & VSO_TYPE_DYN ) {
            virt_msg(".type is VSO_TYPE_DYN, but not VSO_TYPE_SYMLINK, and no .f_get_data for %s", vo->path);
            sysobj_virt_free(vo);
            return FALSE;
        }
        if (vo->type & VSO_TYPE_CONST
            && (vo->type & VSO_TYPE_WRITE || vo->type & VSO_TYPE_WRITE_REQ_ROOT)
            && !vo->f_set_data) {
            virt_msg(".type is VSO_TYPE_CONST, and no .f_set_data for %s", vo->path);
            sysobj_virt_free(vo);
            return FALSE;
        }

        /* search for existing, replace or add */
        g_mutex_lock(&vo_lock);

        if (!(vo->type & VSO_TYPE_DYN)) {
            /* if not DYN, then put it in the tree */
            gboolean existed = FALSE;
            sysobj_virt *tv = g_tree_lookup(vo_tree, vo->path);
            if (tv) existed = TRUE;
            g_tree_replace(vo_tree, g_strdup(vo->path), vo);
            if (existed)
                sysobj_stats.so_virt_replace++;
            else
                sysobj_stats.so_virt_add++;
            g_mutex_unlock(&vo_lock);
            return !existed;
        }

        /* ... else the list */
        for (GSList *l = vo_list; l; l = l->next) {
            sysobj_stats.so_virt_iter++;
            sysobj_virt *lv = l->data;
            if (g_strcmp0(lv->path, vo->path) == 0) {
                /* already exists, overwrite */
                sysobj_virt_free(lv);
                l->data = (gpointer*)vo;
                sysobj_stats.so_virt_replace++;
                g_mutex_unlock(&vo_lock);
                return FALSE;
            }
        }
        //virt_msg("add virtual object to list: %s [%s]", vo->path, vo->str);
        vo_list = g_slist_append(vo_list, vo);
        sysobj_stats.so_virt_add++;
        g_mutex_unlock(&vo_lock);
        return TRUE;
    }
    return FALSE;
}

gboolean sysobj_virt_add_simple_mkpath(const gchar *base, const gchar *name, const gchar *data, int type) {
    gchar *tp = NULL, *tpp = NULL;
    tp = util_build_fn(base, name);
    tpp = g_path_get_dirname(tp);
    if (strlen(tpp) > 1) {
        sysobj_virt *vo = sysobj_virt_find(tpp);
        if (!vo || !SEQ(vo->path, tpp) )
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
    for (int i = 0; lines[i]; i++) {
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
    //virt_msg("find path %s", path);
    sysobj_virt *ret = NULL;
    gchar *spath = g_strdup(path);
    util_null_trailing_slash(spath);
    int best_len = 0;
    /* exact static match wins over longest dynamic match */
    ret = g_tree_lookup(vo_tree, spath);
    if (ret)
        goto sysobj_virt_find_done;

    for (GSList *l = vo_list; l; l = l->next) {
        sysobj_stats.so_virt_iter++;
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

sysobj_virt_find_done:
    g_free(spath);
    //virt_msg("... %s", (ret) ? ret->path : "(NOT FOUND)");
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
            //virt_msg("---\ntarget=%s\nreq=%s\nextry=%s\nret=%s", target, req, extry, ret);
        }
    }
    return ret;
}

gboolean sysobj_virt_set_data(sysobj_virt *vo, const gchar *req, const gpointer data, long length) {
    gboolean ret = FALSE;
    if (vo) {
        int t = sysobj_virt_get_type(vo, req);
        if (!(t & VSO_TYPE_WRITE || t & VSO_TYPE_WRITE_REQ_ROOT) )
            return FALSE;
        if (t & VSO_TYPE_WRITE_REQ_ROOT
            && !util_have_root() )
            return FALSE;

        if (vo->f_set_data) {
            sysobj_stats.so_virt_setf++;
            ret = vo->f_set_data(req ? req : vo->path, data, length);
        } else {
            if (vo->type & VSO_TYPE_CONST)
                return FALSE;
            else if (g_utf8_validate(data, length, NULL) ) {
                if (length <= 0)
                    length = strlen((char*)data);
                g_free(vo->str);
                vo->str = g_memdup(data, length);
                ret = TRUE;
            }
        }
    }
    if (ret)
        sysobj_stats.so_virt_setf_bytes += length;
    return ret;
}

gchar *sysobj_virt_get_data(const sysobj_virt *vo, const gchar *req) {
    gchar *ret = NULL;
    if (vo) {
        if (vo->f_get_data) {
            sysobj_stats.so_virt_getf++;
            ret = vo->f_get_data(req ? req : vo->path);
        } else
            ret = g_strdup(vo->str);

        if (ret && req &&
            vo->type & VSO_TYPE_AUTOLINK ) {
            gchar *res = sysobj_virt_symlink_entry(vo, ret, req);
            g_free(ret);
            ret = res;
        }
    }
    if (ret)
        sysobj_stats.so_virt_getf_bytes += strlen(ret);
    return ret;
}

int sysobj_virt_get_type(const sysobj_virt *vo, const gchar *req) {
    int ret = VSO_TYPE_NONE;
    if (vo) {
        if (vo->f_get_type) {
            ret = vo->f_get_type(req ? req : vo->path);
            if (ret == VSO_TYPE_BASE)
                ret = vo->type;
        } else
            ret = vo->type;
    }
    return ret;
}

typedef struct {
    gchar *spath;
    gsize spl;
    GSList *found;
    gboolean immediate_only;
    gboolean found_name_only; /* list contains: true: name only; false: full path */
} _childs;

/* (GTraverseFunc) */
static gboolean _find_children(const gchar *key, const sysobj_virt *tvo, _childs *ret) {
    sysobj_stats.so_virt_iter++;
    if ( g_str_has_prefix(tvo->path, ret->spath) ) {
        if (ret->immediate_only && strchr(tvo->path + ret->spl, '/'))
            return FALSE;
        if (ret->found_name_only) {
            gchar *fn = g_path_get_basename(tvo->path);
            ret->found = g_slist_append(ret->found, fn);
        } else
            ret->found = g_slist_append(ret->found, g_strdup(tvo->path));
    } else {
        /* tree is traversed in sorted order, so if
         * tvo->path > spath, nothing more could be found */
        if (g_strcmp0(tvo->path, ret->spath) == 1)
            return TRUE; /* stop the foreach */
    }
    return FALSE; /* continue the foreach */
}

GSList *sysobj_virt_all_paths() {
    _childs ret = {};
    ret.spath = "";
    ret.spl = strlen(ret.spath);
    ret.immediate_only = FALSE;
    ret.found_name_only = FALSE;

    /* tree */
    g_tree_foreach(vo_tree, (GTraverseFunc)_find_children, &ret);

    /* list */
    for (GSList *l = vo_list; l; l = l->next) {
        sysobj_stats.so_virt_iter++;
        sysobj_virt *vo = l->data;
        ret.found = g_slist_append(ret.found, g_strdup(vo->path));
    }
    return ret.found;
}

static GSList *sysobj_virt_children_auto(const sysobj_virt *vo, const gchar *req) {
    _childs ret = {};
    if (vo && req) {
        ret.spath = g_strdup_printf("%s/", req);
        ret.spl = strlen(ret.spath);
        ret.immediate_only = TRUE;
        ret.found_name_only = TRUE;

        /* tree */
        g_tree_foreach(vo_tree, (GTraverseFunc)_find_children, &ret);

        /* list */
        gchar *fn = NULL;
        for (GSList *l = vo_list; l; l = l->next) {
            sysobj_stats.so_virt_iter++;
            sysobj_virt *tvo = l->data;
            /* find all vo paths that are immediate children */
            if ( g_str_has_prefix(tvo->path, ret.spath) ) {
                if (!strchr(tvo->path + ret.spl, '/')) {
                    fn = g_path_get_basename(tvo->path);
                    ret.found = g_slist_append(ret.found, fn);
                }
            }
        }
        g_free(ret.spath);
    }
    return ret.found;
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
            for (i = 0; i < len; i++) {
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
    g_tree_destroy(vo_tree);
    g_slist_free_full(vo_list, (GDestroyNotify)sysobj_virt_free);
    vo_tree = NULL;
    vo_list = NULL;
}

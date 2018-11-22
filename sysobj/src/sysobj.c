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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sysobj.h"

gchar sysobj_root[1024] = "";
#define USING_ALT_ROOT (*sysobj_root != 0)
gboolean sysobj_root_set(const gchar *alt_root) {
    gchar *fspath = util_canonicalize_path(alt_root);
    snprintf(sysobj_root, sizeof(sysobj_root) - 1, "%s", fspath);
    util_null_trailing_slash(sysobj_root);
    return TRUE;
}
const gchar *sysobj_root_get() {
    return sysobj_root;
}

static sysobj_filter path_filters[] = {
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE_IIF, "/sys/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/sys", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     ":*", NULL }, /* sysobj_virt */

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/sys", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/sys/*", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/devicetree", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/devicetree/*", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/uptime", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/stat", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/meminfo", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/cpuinfo", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/crypto", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/modules", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/usr/lib/os-release", NULL },

    { SO_FILTER_STATIC | SO_FILTER_EXCLUDE,     "/sys/kernel/security/apparmor*/revision", NULL },

    { SO_FILTER_NONE, "", NULL },
};
static GSList *sysobj_global_filters = NULL;
static GTimer *sysobj_global_timer = NULL;

int compare_str_base10(const sysobj_data *a, const sysobj_data *b) {
    int64_t A = (a && a->str) ? strtol(a->str, NULL, 10) : 0;
    int64_t B = (b && b->str) ? strtol(b->str, NULL, 10) : 0;
    if (A < B) return -1;
    if (A > B) return 1;
    return 0;
}

int compare_str_base16(const sysobj_data *a, const sysobj_data *b) {
    int64_t A = (a && a->str) ? strtol(a->str, NULL, 16) : 0;
    int64_t B = (b && b->str) ? strtol(b->str, NULL, 16) : 0;
    if (A < B) return -1;
    if (A > B) return 1;
    return 0;
}

GSList *class_list = NULL;
GSList *vo_list = NULL;

GSList *class_get_list() {
    return class_list;
}

void class_free(sysobj_class *s) {
    if (s) {
        if (s->f_cleanup)
            s->f_cleanup();
        if (!(s->flags & OF_CONST) )
            g_free(s);
    }
}

void class_cleanup() {
    g_slist_free_full(class_list, (GDestroyNotify)class_free);
}

const gchar *simple_label(sysobj* obj) {
    if (obj && obj->cls) {
        return _(obj->cls->s_label);
    }
    return NULL;
}

const gchar *simple_halp(sysobj* obj) {
    if (obj && obj->cls) {
        return obj->cls->s_halp;
    }
    return NULL;
}

gboolean sysobj_exists(sysobj *s) {
    if (s) {
        sysobj_fscheck(s);
        return s->exists;
    }
    return FALSE;
}

gboolean sysobj_exists_from_fn(const gchar *base, const gchar *name) {
    gboolean exists = FALSE;
    sysobj *obj = sysobj_new_from_fn(base, name);
    exists = sysobj_exists(obj);
    sysobj_free(obj);
    return exists;
}

gchar *sysobj_format_from_fn(const gchar *base, const gchar *name, int fmt_opts) {
    gchar *ret = NULL;
    sysobj *obj = sysobj_new_from_fn(base, name);
    ret = sysobj_format(obj, fmt_opts);
    sysobj_free(obj);
    return ret;
}

gchar *sysobj_raw_from_fn(const gchar *base, const gchar *name) {
    gchar *ret = NULL;
    sysobj *obj = sysobj_new_from_fn(base, name);
    sysobj_read(obj, FALSE);
    ret = g_strdup(obj->data.str);
    sysobj_free(obj);
    return ret;
}

uint32_t sysobj_uint32_from_fn(const gchar *base, const gchar *name, int nbase) {
    gchar *tmp = sysobj_raw_from_fn(base, name);
    uint32_t ret = tmp ? strtol(tmp, NULL, nbase) : 0;
    g_free(tmp);
    return ret;
}

#define TERM_COLOR_FMT(CLR) ((fmt_opts & FMT_OPT_ATERM) ? (CLR "%s" ANSI_COLOR_RESET) : "%s")

gchar *simple_format(sysobj* obj, int fmt_opts) {
    static const gchar *special[] = {
        N_("{permission denied}"),
        N_("{node}"),
        N_("{binary value}"),
        N_("{not found}"),
        N_("{empty}"),
    };

    if (obj) {
        gchar *nice = NULL;
        if ( obj->access_fail )
            nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_RED), special[0] );
        else if ( obj->data.is_dir )
            nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_BLUE), special[1] );
        else if ( !obj->data.is_utf8 )
            if ( obj->data.len == 0 )
                nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_YELLOW), special[4] );
            else
                nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_YELLOW), special[2] );
        else if ( !obj->exists || !obj->data.str ) {
            if (fmt_opts & FMT_OPT_NULL_IF_MISSING)
                nice = NULL;
            else
                nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_RED), special[3] );
        } else {
            if ( *(obj->data.str) == 0
                && ( fmt_opts & FMT_OPT_NULL_IF_EMPTY) )
                nice = NULL;
            else {
                gchar *text = NULL;
                if (fmt_opts & FMT_OPT_LIST_ITEM &&
                    (obj->data.lines > 1 || obj->data.len > 120) )
                    text = g_strdup_printf("{%lu line(s) text, utf8}", obj->data.lines);
                else
                    text = g_strdup(obj->data.str);

                g_strchomp(text);

                if (fmt_opts & FMT_OPT_HTML
                    || fmt_opts & FMT_OPT_PANGO) {
                    nice = util_escape_markup(text, FALSE);
                    g_free(text);
                } else
                    nice = text;
            }
        }
        return nice;
    }
    return NULL;
}

const sysobj_class *class_add(sysobj_class *c) {
    if (c)
        class_list = g_slist_append(class_list, c);
    return c;
}

const sysobj_class *class_add_full(sysobj_class *base,
    const gchar *tag, const gchar *pattern,
    const gchar *s_label, const gchar *s_halp, guint flags,
    double s_update_interval,
    void *f_verify, void *f_label, void *f_halp,
    void *f_format, void *f_update_interval, void *f_compare,
    void *f_flags ) {

    sysobj_class *nc = g_new0(sysobj_class, 1);
#define CLASS_PROVIDE_OR_INHERIT(M) nc->M = M ? M : (base ? base->M : 0 )
    CLASS_PROVIDE_OR_INHERIT(tag);
    CLASS_PROVIDE_OR_INHERIT(pattern);
    CLASS_PROVIDE_OR_INHERIT(s_label);
    CLASS_PROVIDE_OR_INHERIT(s_halp);
    CLASS_PROVIDE_OR_INHERIT(s_update_interval);
    CLASS_PROVIDE_OR_INHERIT(flags);
    CLASS_PROVIDE_OR_INHERIT(f_verify);
    CLASS_PROVIDE_OR_INHERIT(f_label);
    CLASS_PROVIDE_OR_INHERIT(f_halp);
    CLASS_PROVIDE_OR_INHERIT(f_format);
    CLASS_PROVIDE_OR_INHERIT(f_update_interval);
    CLASS_PROVIDE_OR_INHERIT(f_compare);
    CLASS_PROVIDE_OR_INHERIT(f_flags);

    if (nc->pattern)
        return class_add(nc);
    else
        g_free(nc);

    return NULL;
}

const sysobj_class *class_add_simple(const gchar *pattern, const gchar *label, const gchar *tag, guint flags, double update_interval) {
    return class_add_full(NULL, tag, pattern, label, NULL, flags, update_interval, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

gboolean sysobj_has_flag(sysobj *s, guint flag) {
    if (s) {
        const sysobj_class *c = s->cls;
        if (c) {
            guint f = c->flags;
            if (c->f_flags)
                f = c->f_flags(s);
            if (f & flag)
                return TRUE;
        }
    }
    return FALSE;
}

gboolean class_has_flag(const sysobj_class *c, guint flag) {
    if (c) {
        guint f = c->flags;
        if (c->f_flags)
            f = c->f_flags(NULL);
        if (f & flag)
            return TRUE;
    }
    return FALSE;
}

sysobj_data *sysobj_data_dup(const sysobj_data *src) {
    if (src) {
        sysobj_data *dest = g_memdup(src, sizeof(sysobj_data));
        /* +1 because g_file_get_contents() always adds \0 */
        dest->any = g_memdup(src->any, src->len + 1);
        return dest;
    }
    return NULL;
}

sysobj *sysobj_new() {
    sysobj *s = g_new0(sysobj, 1);
    return s;
}

sysobj *sysobj_dup(const sysobj *src) {
    sysobj *ret = g_memdup(src, sizeof(sysobj_data));
    /* strings */
    ret->path_req_fs = g_strdup(src->path_req_fs);
    ret->name_req = g_strdup(src->name_req);
    ret->path_fs = g_strdup(src->path_fs);
    ret->name = g_strdup(src->name);

    /* pointers into strings */
    ret->path = ret->path_fs + (src->path - src->path_fs);
    ret->path_req = ret->path_req_fs + (src->path_req - src->path_req_fs);

    /* data */
    ret->data.any = g_memdup(ret->data.any, ret->data.len + 1);

    return ret;
}

void sysobj_data_free(sysobj_data *d, gboolean and_self) {
    if (d) {
        g_free(d->any);
        g_slist_free_full(d->childs, (GDestroyNotify)g_free);
        if (and_self)
            g_free(d);
    }
}

/* keep the pointer, but make it like the result of sysobj_new() */
static void sysobj_free_and_clean(sysobj *s) {
    if (s) {
        g_free(s->path_req_fs);
        g_free(s->name_req);
        g_free(s->path_fs);
        g_free(s->name);
        sysobj_data_free(&s->data, FALSE);
        memset(s, 0, sizeof(sysobj) );
    }
}

void sysobj_free(sysobj *s) {
    if (s) {
        sysobj_free_and_clean(s);
        g_free(s);
    }
}

void sysobj_classify(sysobj *s) {
    GSList *l = class_list;
    sysobj_class *c;
    if (s) {
        while (l) {
            c = l->data;
            gboolean match = FALSE;

            if ( class_has_flag(c, OF_GLOB_PATTERN) )
                match = g_pattern_match_simple(c->pattern, s->path);
            else
                match = g_str_has_suffix(s->path, c->pattern);

            if (match && c->f_verify)
                match = c->f_verify(s);

            if (match) {
                s->cls = c;
                return;
            }
            l = l->next;
        }
    }
}

void sysobj_fscheck(sysobj *s) {
    if (s && s->path) {
        s->exists = FALSE;
        s->root_can_read = FALSE;
        s->root_can_write = FALSE;
        s->others_can_read = FALSE;
        s->others_can_write = FALSE;
        s->data.is_dir = FALSE;

        if (*(s->path) == ':') {
            /* virtual */
            const sysobj_virt *vo = sysobj_virt_find(s->path);
            if (vo) {
                s->exists = TRUE;
                s->root_can_read = TRUE;
                int t = sysobj_virt_get_type(vo, s->path);
                s->others_can_read = (t & VSO_TYPE_REQ_ROOT) ? FALSE : TRUE;
                if (t & VSO_TYPE_DIR)
                    s->data.is_dir = TRUE;
            }
        } else {
            s->exists = g_file_test(s->path_fs, G_FILE_TEST_EXISTS);
            if (s->exists) {
                s->data.is_dir = g_file_test(s->path_fs, G_FILE_TEST_IS_DIR);
                struct stat fst;
                if (stat(s->path_fs, &fst) != -1 ) {
                    if (fst.st_mode & S_IFIFO) s->is_pipe = TRUE;
                    if (fst.st_mode & S_IRUSR) s->root_can_read = TRUE;
                    if (fst.st_mode & S_IWUSR) s->root_can_write = TRUE;
                    if (fst.st_mode & S_IROTH) s->others_can_read = TRUE;
                    if (fst.st_mode & S_IWOTH) s->others_can_write = TRUE;
                }
            }
        }
    }
}

static GSList *sysobj_virt_children(const sysobj_virt *vo, const gchar *req);
static void sysobj_read_dir(sysobj *s) {
    GSList *nl = NULL;
    GDir *dir;
    const gchar *fn;

    if (!s) return;

    if (*(s->path) == ':') {
        /* virtual */
        const sysobj_virt *vo = sysobj_virt_find(s->path);
        if (vo) {
            nl = sysobj_virt_children(vo, s->path);
            s->data.was_read = TRUE;
        }
    } else {
        /* normal */
        dir = g_dir_open(s->path_fs, 0 , NULL);
        if (dir) {
            while((fn = g_dir_read_name(dir)) != NULL) {
                nl = g_slist_append(nl, g_strdup(fn));
            }
            g_dir_close(dir);
            s->data.was_read = TRUE;
        }
    }
    s->data.childs = nl;
}

static void sysobj_read_data(sysobj *s) {
    GError *error = NULL;

    if (!s) return;

    if (*(s->path) == ':') {
        /* virtual */
        s->data.was_read = FALSE;
        const sysobj_virt *vo = sysobj_virt_find(s->path);
        if (vo) {
            gboolean readable = s->others_can_read;
            if (!readable) {
                if (s->root_can_read && util_have_root() )
                    readable = TRUE;
            }

            if (readable) {
                s->data.str = sysobj_virt_get_data(vo, s->path);
                s->data.was_read = TRUE;
                if (s->data.str) {
                    s->data.len = strlen(s->data.str); //TODO: what if not c-string?
                } else
                    s->exists = FALSE;
            } else
                s->access_fail = TRUE;
        }
    } else {
        /* normal */
        s->data.was_read =
            g_file_get_contents(s->path_fs, &s->data.str, &s->data.len, &error);
            if (!s->data.str) {
                if (error && error->code == G_FILE_ERROR_ACCES)
                    s->access_fail = TRUE;
            }
            if (error)
                g_error_free(error);
    }

    if (s->data.was_read) {
        s->data.is_utf8 = g_utf8_validate(s->data.str, s->data.len, NULL);
        if (s->data.is_utf8) {
            s->data.lines = util_count_lines(s->data.str);
            s->data.maybe_num = util_maybe_num(s->data.str);
        }
    }
}

gboolean sysobj_read(sysobj *s, gboolean force) {
    if (s && s->path) {
        if (!force && s->data.was_read) {
            if (!sysobj_data_expired(s) )
                return FALSE;
        }

        if (s->data.is_dir)
            sysobj_read_dir(s);
        else
            sysobj_read_data(s);

        s->data.stamp = sysobj_elapsed();
        return TRUE;
    }
    return FALSE;
}

void sysobj_unread_data(sysobj *s) {
    if (s) {
        if (s->data.was_read && s->data.any) {
            g_free(s->data.any);
            s->data.any = NULL;
        }
        s->data.was_read = FALSE;
    }
}

double sysobj_update_interval(sysobj *s) {
    if (s) {
        if (s->cls && s->cls->f_update_interval)
            return s->cls->f_update_interval(s);
        if (s->cls && s->cls->s_update_interval)
            return s->cls->s_update_interval;
        return UPDATE_INTERVAL_DEFAULT;
    }
    return UPDATE_INTERVAL_NEVER;
}

/* sysobj dev names are commonly nameN. Sort so that
 * name9 comes before name10 and name4part6 before
 * name4part14. */
int sysfs_fn_cmp(gchar *a, gchar *b) {
    /* one or both are null */
    if (!a) return b ? -1 : 0;
    if (!b) return 1;
    gchar *pa = a, *pb = b;
    while(pa && pb) {
        int mc = 0, na = 0, nb = 0, l = 0, cmp = 0;
        char buff[64] = "";
        mc = sscanf(pa, "%[^0-9]%d", buff, &na);
        if (mc == 2) {
            l = strlen(buff);
            if (!l) return strcmp(pa, pb);
            cmp = strncmp(pb, buff, l);
            if (cmp != 0) return strcmp(pa, pb);
            pa += l; pb += l;
            mc = sscanf(pb, "%d", &nb);
            if (mc != 1) return strcmp(pa, pb);
            cmp = na - nb;
            if (cmp != 0) return cmp;
            /* skip past the numbers */
            while(isdigit(*pa)) pa++;
            while(isdigit(*pb)) pb++;

        } else
            return strcmp(pa, pb);
    }
    return 0;
}

/* because g_slist_copy_deep( .. , g_strdup, NULL)
 * would have been too easy */
static gchar *dumb_string_copy(gchar *src, gpointer *e) {
    PARAM_NOT_UNUSED(e);
    return g_strdup(src);
}

GSList *sysobj_children_ex(sysobj *s, GSList *filters, gboolean sort) {
    GSList *ret = NULL;

    if (s) {
        sysobj_read(s, FALSE);
        ret = g_slist_copy_deep(s->data.childs, (GCopyFunc)dumb_string_copy, NULL);

        if (filters)
            ret = sysobj_filter_list(ret, filters);

        if (sort)
            ret = g_slist_sort(ret, (GCompareFunc)sysfs_fn_cmp);
    }
    return ret;
}

GSList *sysobj_children(sysobj *s, gchar *include_glob, gchar *exclude_glob, gboolean sort) {
    GSList *filters = NULL, *ret = NULL;
    if (include_glob)
        filters = g_slist_append(filters, sysobj_filter_new(SO_FILTER_INCLUDE_IIF, include_glob) );
    if (exclude_glob)
        filters = g_slist_append(filters, sysobj_filter_new(SO_FILTER_EXCLUDE_IIF, exclude_glob) );
    ret = sysobj_children_ex(s, filters, sort);
    g_slist_free_full(filters, (GDestroyNotify)sysobj_filter_free);
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

gboolean sysobj_config_paths(sysobj *s, const gchar *base, const gchar *name) {
    gchar *req = NULL, *norm = NULL, *vlink = NULL, *chkpath = NULL, *fspath = NULL;
    gboolean target_is_real = FALSE, req_is_real = FALSE;
    gchar *vlink_tmp = NULL;
    int vlr_count = 0;

    if (!s) return FALSE;

    if (name)
        req = g_strdup_printf("%s/%s", base, name);
    else
        req = strdup(base);
    util_null_trailing_slash(req);
    norm = util_normalize_path(req, "/");
    g_free(req);
    if (g_str_has_prefix(norm, "/:")) {
        req = g_strdup(norm+1);
        g_free(norm);
    } else
        req = norm;
    /* norm is now used or freed */

    /* virtual symlink */
    req_is_real = (*req == ':') ? FALSE : TRUE;

    if (!req_is_real) {
        vlink = sysobj_virt_is_symlink(req);
        while (vlink && *vlink == ':' && vlr_count < 10) {
            vlink_tmp = sysobj_virt_is_symlink(vlink);
            if (vlink_tmp) {
                g_free(vlink);
                vlink = vlink_tmp;
                vlr_count++;
            } else break;
        }
    }

    if (vlink)
        target_is_real = (*vlink == ':') ? FALSE : TRUE;
    else
        target_is_real = (*req == ':') ? FALSE : TRUE;

    /* canonicalize: follow all symlinks if target is real */
    if (target_is_real) {
        if (USING_ALT_ROOT) {
            gchar chkchr = vlink ? *vlink : *req;
            chkpath = g_strdup_printf("%s%s%s", sysobj_root, (chkchr == '/') ? "" : "/", vlink ? vlink : req);
        } else
            chkpath = g_strdup(vlink ? vlink : req);

        fspath = util_canonicalize_path(chkpath);
    } else {
        fspath = g_strdup(vlink ? vlink : req);
    }

    if (!fspath)
        fspath = chkpath; /* the path may not exist */
    else
        g_free(chkpath);

    /* set object paths */
    s->path_req = s->path_req_fs = req;
    s->path = s->path_fs = fspath;
    if (USING_ALT_ROOT) {
        int alt_root_len = strlen(sysobj_root);
        if (target_is_real) {
            /* check for .. beyond sysobj_root */
            if (!g_str_has_prefix(s->path_fs, sysobj_root) )
                goto config_bad_path;

            s->path = s->path_fs + alt_root_len;
        }
    }

    if (vlink)
        s->req_is_link = TRUE;
    else
        s->req_is_link = g_file_test(s->path_req_fs, G_FILE_TEST_IS_SYMLINK);

    //DEBUG("\n{ .path_req_fs = %s\n  .path_req = %s\n  .path_fs = %s\n  .path = %s\n  .req_is_link = %s }",
    //    s->path_req_fs, s->path_req, s->path_fs, s->path, s->req_is_link ? "TRUE" : "FALSE" );

    if (!sysobj_filter_item_include(s->path, sysobj_global_filters) ) {
        goto config_bad_path;
    }

    g_free(vlink);
    return TRUE;

config_bad_path:
    DEBUG("BAD PATH: %s -> %s (%s)", s->path_req, s->path, s->path_fs);
    g_free(fspath);
    g_free(vlink);
    s->path = s->path_fs = g_strdup(":error/bad_path");
    return FALSE;
}

sysobj *sysobj_new_from_fn(const gchar *base, const gchar *name) {
    sysobj *s = NULL;
    if (base) {
        s = sysobj_new();
        sysobj_config_paths(s, base, name);
        s->name_req = g_path_get_basename(s->path_req);
        s->name = g_path_get_basename(s->path);
        sysobj_fscheck(s);
        sysobj_classify(s);
    }
    return s;
}

gchar *sysobj_parent_path(sysobj *s) {
    if (s) {
        gchar *pp = g_path_get_dirname(s->path);
        return pp;
    }
    return NULL;
}

gchar *sysobj_parent_name(sysobj *s) {
    if (s) {
        gchar *pp = sysobj_parent_path(s);
        if (pp) {
            gchar *pn = g_path_get_basename(pp);
            g_free(pp);
            return pn;
        }
    }
    return NULL;
}

const gchar *sysobj_label(sysobj *s) {
        if (s && s->cls && s->cls->f_label) {
            return s->cls->f_label(s);
        }
        return simple_label(s);
}

const gchar *sysobj_halp(sysobj *s) {
        if (s && s->cls && s->cls->f_halp) {
            return s->cls->f_halp(s);
        }
        return simple_halp(s);
}

gchar *sysobj_format(sysobj *s, int fmt_opts) {
    if (s) {
        sysobj_read(s, FALSE);
        if (s->cls && s->cls->f_format)
            return s->cls->f_format(s, fmt_opts);
        else
            return simple_format(s, fmt_opts);
    }
    return NULL;
}

void class_dump_list() {
    GSList *l = class_list;
    sysobj_class *c = NULL;
    PARAM_NOT_UNUSED(c);
    while (l) {
        c = l->data;
        DEBUG("class {%s} [ %s ] 0x%x", c->tag ? c->tag : "none", c->pattern, c->flags);
        l = l->next;
    }
}

gboolean verify_parent(sysobj *obj, const gchar *parent_path_suffix) {
    gboolean verified = FALSE;
    if (obj && obj->name) {
        gchar *pp = sysobj_parent_path(obj);
        if (g_str_has_suffix(pp, parent_path_suffix))
            verified = TRUE;
        g_free(pp);
    }
    return verified;
}

gboolean verify_parent_name(sysobj *obj, const gchar *parent_name) {
    gboolean verified = FALSE;
    if (obj && obj->name) {
        gchar *pn = sysobj_parent_name(obj);
        if (!strcmp(pn, parent_name))
            verified = TRUE;
        g_free(pn);
    }
    return verified;
}

gboolean verify_lblnum(sysobj *obj, const gchar *lbl) {
    if (obj && obj->name
        && lbl && util_get_did(obj->name, lbl) >= 0)
        return TRUE;
    return FALSE;
}

gboolean verify_lblnum_child(sysobj *obj, const gchar *lbl) {
    gboolean verified = FALSE;
    if (lbl && obj && obj->name) {
        gchar *parent_name = sysobj_parent_name(obj);
        if ( util_get_did(parent_name, lbl) >= 0 )
            verified = TRUE;
        g_free(parent_name);
    }
    return verified;
}

static int virt_path_cmp(sysobj_virt *a, sysobj_virt *b) {
    /* one or both are null */
    if (!a) return b ? -1 : 0;
    if (!b) return 1;
    return g_strcmp0(a->path, b->path);
}

void sysobj_virt_remove(gchar *glob) {
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
}

gboolean sysobj_virt_add(sysobj_virt *vo) {
    if (vo) {
        GSList *l = vo_list;
        while(l) {
            sysobj_virt *lv = l->data;
            if (g_strcmp0(lv->path, vo->path) == 0) {
                /* already exists, overwrite */
                sysobj_virt_free(lv);
                l->data = (gpointer*)vo;
                return FALSE;
            }
            l = l->next;
        }
        //printf("add virtual object: %s [%s]\n", vo->path, vo->str);
        vo_list = g_slist_append(vo_list, vo);
        return TRUE;
    }
    return FALSE;
}

gboolean sysobj_virt_add_simple_mkpath(const gchar *base, const gchar *name, const gchar *data, int type) {
    gchar *tp = NULL, *tpp = NULL;
    if (name)
        tp = g_strdup_printf("%s/%s", base, name);
    else
        tp = g_strdup(base);

    tpp = g_path_get_dirname(tp);
    if (strlen(tpp) > 1)
        sysobj_virt_add_simple_mkpath(tpp, NULL, "*", VSO_TYPE_DIR);
    g_free(tp);
    g_free(tpp);
    return sysobj_virt_add_simple(base, name, data, type);
}

gboolean sysobj_virt_add_simple(const gchar *base, const gchar *name, const gchar *data, int type) {
    sysobj_virt *vo = g_new0(sysobj_virt, 1);
    if (name)
        vo->path = g_strdup_printf("%s/%s", base, name);
    else
        vo->path = g_strdup(base);
    vo->type = type;
    vo->str = g_strdup(data);
    return sysobj_virt_add(vo);
}

void sysobj_virt_from_kv(gchar *base, const gchar *kv_data_in) {
    const gchar before_first_group[] = "THE_NO-GROUP________GRUOP";
    GKeyFile *key_file = NULL;
    gchar **groups = NULL, **keys = NULL;
    int i = 0, j = 0;

    gchar *kv_data = g_strdup_printf("[%s]\n%s", before_first_group, kv_data_in);

    key_file = g_key_file_new();
    g_key_file_load_from_data(key_file, kv_data, strlen(kv_data), 0, NULL);
    groups = g_key_file_get_groups(key_file, NULL);
    for (i = 0; groups[i]; i++) {
        gboolean is_bs_group = (!strcmp(groups[i], before_first_group)) ? TRUE : FALSE;

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
    DEBUG("find path %s", path);
    sysobj_virt *ret = NULL;
    gchar *spath = g_strdup(path);
    util_null_trailing_slash(spath);
    GSList *l = vo_list;
    while (l) {
        sysobj_virt *vo = l->data;
        if (vo->type & VSO_TYPE_DYN && g_str_has_prefix(path, vo->path) ) {
            ret = vo;
            /* break; -- No, don't break, maybe a non-dynamic item matches */
        }
        if (strcmp(vo->path, path) == 0) {
            ret = vo;
            break;
        }
        l = l->next;
    }
    g_free(spath);
    DEBUG("... %s", (ret) ? ret->path : "(NOT FOUND)");
    return ret;
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

static GSList *sysobj_virt_children(const sysobj_virt *vo, const gchar *req) {
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
    if (s && !(s->type & VSO_TYPE_CONST)) {
        g_free(s->path);
        g_free(s->str);
        g_free(s);
    }
}

void sysobj_virt_cleanup() {
    g_slist_free_full(vo_list, (GDestroyNotify)sysobj_virt_free);
}

void sysobj_init(const gchar *alt_root) {
    if (alt_root)
        sysobj_root_set(alt_root);

    int i = 0;
    while(path_filters[i].type != SO_FILTER_NONE) {
        sysobj_global_filters = g_slist_append(sysobj_global_filters, &path_filters[i]);
        i++;
    }

    sysobj_global_timer = g_timer_new();
    g_timer_start(sysobj_global_timer);

    class_init();
}

double sysobj_elapsed() {
    return g_timer_elapsed(sysobj_global_timer, NULL);
}

void sysobj_cleanup() {
    class_cleanup();
    sysobj_virt_cleanup();
    g_timer_destroy(sysobj_global_timer);
}

sysobj_filter *sysobj_filter_new(int type, gchar *pattern) {
    sysobj_filter *f = g_new0(sysobj_filter, 1);
    f->type = type;
    f->pspec = g_pattern_spec_new(pattern);
    return f;
}

void sysobj_filter_free(sysobj_filter *f) {
    if (f) {
        if (f->pspec)
            g_pattern_spec_free(f->pspec);

        if (!(f->type & SO_FILTER_STATIC) ) {
            g_free(f->pattern);
            g_free(f);
        }
    }
}

gboolean sysobj_filter_item_include(gchar *item, GSList *filters) {
    if (!item) return FALSE;

    sysobj_filter *f = NULL;
    gboolean marked = FALSE;
    GSList *fp = filters;
    gsize len = strlen(item);

    while (fp) {
        f = fp->data;
        if (!f->pspec)
            f->pspec = g_pattern_spec_new(f->pattern);
        gboolean match = g_pattern_match(f->pspec, len, item, NULL);
        switch (f->type & SO_FILTER_MASK) {
            case SO_FILTER_EXCLUDE_IIF:
                marked = match;
                break;
            case SO_FILTER_INCLUDE_IIF:
                marked = !match;
                break;
            case SO_FILTER_EXCLUDE:
                if (match) marked = TRUE;
                break;
            case SO_FILTER_INCLUDE:
                if (match) marked = FALSE;
                break;
        }

        /*
        DEBUG(" pattern: %s %s%s -> %s -> %s", f->pattern,
            f->type & SO_FILTER_EXCLUDE ? "ex" : "inc", f->type & SO_FILTER_IIF ? "_iif" : "",
            match ? "match" : "no-match", marked ? "marked" : "not-marked"); */

        fp = fp->next;
    }

    return !marked;
}

GSList *sysobj_filter_list(GSList *items, GSList *filters) {
    GSList *fp = filters, *l = NULL, *n = NULL;
    sysobj_filter *f = NULL;
    gboolean *marked;
    gsize i = 0, num_items = g_slist_length(items);

    marked = g_new(gboolean, num_items);
    while (fp) {
        f = fp->data;
        if (!f->pspec)
            f->pspec = g_pattern_spec_new(f->pattern);
        l = items;
        i = 0;
        while (l) {
            gchar *d = l->data;
            gsize len = strlen(d);
            gboolean match = g_pattern_match(f->pspec, len, d, NULL);
            switch (f->type) {
                case SO_FILTER_EXCLUDE_IIF:
                    marked[i] = match;
                    break;
                case SO_FILTER_INCLUDE_IIF:
                    marked[i] = !match;
                    break;
                case SO_FILTER_EXCLUDE:
                    if (match) marked[i] = TRUE;
                    break;
                case SO_FILTER_INCLUDE:
                    if (match) marked[i] = FALSE;
                    break;
            }
            l = l->next;
            i++;
        }
        fp = fp->next;
    }

    l = items;
    i = 0;
    while (l) {
        n = l->next;
        if (marked[i])
            items = g_slist_delete_link(items, l);
        l = n;
        i++;
    }

    g_free(marked);
    return items;
}

/* table is null-terminated string list of null-terminated UNTRANSLATED strings
 * table_len, optional, improves speed
 * nbase is the base of the number stored in obj->data.str
 * fmt_opts, as usual */
gchar *sysobj_format_table(sysobj *obj, gchar **table, int table_len, int nbase, int fmt_opts) {
    gchar *val = NULL;
    int index = 0, count = 0;

    FMT_OPTS_IGNORE();

    if (obj && obj->data.str && isxdigit(*(obj->data.str)) ) {
        index = strtol(obj->data.str, NULL, nbase);
        if (table_len) {
            if (index < table_len)
                val = table[index];
        } else {
            gchar **p = table;
            while(p) {
                if (count == index) {
                    val = *p;
                    break;
                }
                count++;
                p++;
            }
        }
        if (val)
            return g_strdup_printf("[%d] %s", index, _(val));
        return g_strdup_printf("[%d]", index);
    }
    return simple_format(obj, fmt_opts);
}

sysobj *sysobj_parent(sysobj *s) {
    if (s) {
        gchar *rpp = g_path_get_dirname(s->path_req);
        sysobj *ret = sysobj_new_from_fn(rpp, NULL);
        g_free(rpp);
        return ret;
    }
    return NULL;
}

sysobj *sysobj_child(sysobj *s, gchar *child) {
    return sysobj_new_from_fn(s->path_req, child);
}

sysobj *sysobj_child_of_parent(sysobj *s, gchar *child_path) {
    if (s) {
        gchar *rpp = g_path_get_dirname(s->path_req);
        sysobj *ret = sysobj_new_from_fn(rpp, child_path);
        g_free(rpp);
        return ret;
    }
    return NULL;
}

const gchar *sysobj_suggest(sysobj *s) {
    if (s && s->cls && s->cls->s_suggest) {
        return s->cls->s_suggest;
    }
    return NULL;
}

gboolean sysobj_data_expired(sysobj *s) {
    if (s) {
        double ui = sysobj_update_interval(s);
        if (ui == UPDATE_INTERVAL_NEVER)
            return !s->data.was_read; /* once read, never expires */

        if ( (sysobj_elapsed() - s->data.stamp) >=  ui ) {
            return TRUE;
        }
    }
    return FALSE;
}

int class_count() {
    return g_slist_length(class_list);
}

int sysobj_virt_count() {
    return g_slist_length(vo_list);
}

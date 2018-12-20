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
#include "gg_file.h"
#include "sysobj.h"
#include "vendor.h"

so_stats sysobj_stats = {};
GSList *sysobj_data_paths = NULL;

gchar *sysobj_find_data_file(const gchar *file) {
    for (GSList *l = sysobj_data_paths; l; l = l->next) {
        gchar *chk = util_build_fn(l->data, file);
        DEBUG("checking: %s", chk);
        if (g_file_test(chk, G_FILE_TEST_EXISTS) )
            return chk;
        g_free(chk);
    }
    return NULL;
}

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

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/driver", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/driver/*", NULL },

    /* files related to os version detection (:/os) */
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/etc/*-release", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/etc/*_version", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/etc/*-version", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/etc/issue", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/usr/lib/os-release", NULL },

    /* things in here can break the whole universe if read */
    { SO_FILTER_STATIC | SO_FILTER_EXCLUDE,     "/sys/kernel/debug/*", NULL },

    { SO_FILTER_NONE, "", NULL }, /* end of list */
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

static GMutex free_lock;
static GSList *free_list = NULL;
static guint free_event_source = 0;
static gboolean free_final = FALSE;

typedef struct {
    gpointer ptr;
    GThread *thread;
    GDestroyNotify f_free;
    double stamp;

    const char *file;
    int  line;
    const char *func;
} auto_free_item;

gpointer auto_free_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func) {
    if (!p) return p;

    auto_free_item *z = g_new0(auto_free_item, 1);
    z->ptr = p;
    z->f_free = f;
    z->thread = g_thread_self();
    z->file = file;
    z->line = line;
    z->func = func;
    z->stamp = sysobj_elapsed();
    g_mutex_lock(&free_lock);
    free_list = g_slist_prepend(free_list, z);
    sysobj_stats.auto_free_len++;
    g_mutex_unlock(&free_lock);
    return p;
}

gpointer auto_free_(gpointer p, const char *file, int line, const char *func) {
    return auto_free_ex_(p, (GDestroyNotify)g_free, file, line, func);
}

static struct { GDestroyNotify fptr; char *name; }
    free_function_tab[] = {
    { (GDestroyNotify) g_free,             "g_free" },
    { (GDestroyNotify) sysobj_free,        "sysobj_free" },
    { (GDestroyNotify) class_free,         "class_free" },
    { (GDestroyNotify) sysobj_filter_free, "sysobj_filter_free" },
    { (GDestroyNotify) sysobj_virt_free,   "sysobj_virt_free" },
    { NULL, "(null)" },
};

void free_auto_free_final() {
    free_final = TRUE;
    free_auto_free();
}

void free_auto_free() {
    GThread *this_thread = g_thread_self();
    GSList *l = NULL, *n = NULL;
    long long unsigned fc = 0;
    double now = sysobj_elapsed();

    if (!free_list) return;

    g_mutex_lock(&free_lock);
    DEBUG("%llu total items in queue, but will free from thread %p only... ", sysobj_stats.auto_free_len, this_thread);
    for(l = free_list; l; l = n) {
        auto_free_item *z = (auto_free_item*)l->data;
        n = l->next;
        double age = now - z->stamp;
        if (free_final || (z->thread == this_thread && age > AF_DELAY_SECONDS) ) {
            if (DEBUG_AUTO_FREE) {
                char fptr[128] = "", *fname;
                for(int i = 0; i < (int)G_N_ELEMENTS(free_function_tab); i++)
                    if (z->f_free == free_function_tab[i].fptr)
                        fname = free_function_tab[i].name;
                if (!fname) {
                    snprintf(fname, 127, "%p", z->f_free);
                    fname = fptr;
                }
                if (z->file || z->func)
                    DEBUG("free: %s(%p) age:%lfs from %s:%d %s()", fname, z->ptr, age, z->file, z->line, z->func);
                else
                    DEBUG("free: %s(%p) age:%lfs", fname, z->ptr, age);
            }

            z->f_free(z->ptr);
            g_free(z);
            free_list = g_slist_delete_link(free_list, l);
            fc++;
        }
    }
    DEBUG("... freed %llu (from thread %p)", fc, this_thread);
    sysobj_stats.auto_freed += fc;
    sysobj_stats.auto_free_len -= fc;
    g_mutex_unlock(&free_lock);
}

gboolean free_auto_free_sf(gpointer trash) {
    PARAM_NOT_UNUSED(trash);
    free_auto_free();
    sysobj_stats.auto_free_next = sysobj_elapsed() + AF_SECONDS;
    return G_SOURCE_CONTINUE;
}

GSList *class_get_list() {
    return class_list;
}

void class_free(sysobj_class *s) {
    if (s) {
        if (s->f_cleanup)
            s->f_cleanup();
        if (s->pspec) {
            g_pattern_spec_free(s->pspec);
            s->pspec = NULL;
        }
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
    gchar *path = util_build_fn(base, name);
    sysobj *obj = sysobj_new_fast(path);
    exists = sysobj_exists(obj);
    sysobj_free(obj);
    g_free(path);
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
    gchar *req = util_build_fn(base, name);
    sysobj *obj = sysobj_new_fast(req);
    sysobj_read(obj, FALSE);
    ret = g_strdup(obj->data.str);
    sysobj_free(obj);
    g_free(req);
    return ret;
}

gchar *sysobj_raw_from_printf(gchar *path_fmt, ...) {
    gchar *ret = NULL;
    gchar *path = NULL;
    va_list args;
    va_start(args, path_fmt);
    path = g_strdup_vprintf(path_fmt, args);
    va_end(args);
    ret = sysobj_raw_from_fn(path, NULL);
    g_free(path);
    return ret;
}

uint32_t sysobj_uint32_from_fn(const gchar *base, const gchar *name, int nbase) {
    gchar *tmp = sysobj_raw_from_fn(base, name);
    uint32_t ret = tmp ? strtol(tmp, NULL, nbase) : 0;
    g_free(tmp);
    return ret;
}

gchar *simple_format(sysobj* obj, int fmt_opts) {
    const gchar *msg = NULL;
    gchar *text = NULL, *nice = NULL;

    if (!obj) return NULL;

    const gchar *str = obj->data.str;
    gboolean
        exists = obj->exists,
        fail   = obj->access_fail,
        utf8   = obj->data.is_utf8,
        dir    = obj->data.is_dir,
        empty  = (obj->data.len == 0)
            || (utf8 && strlen(text = g_strstrip(g_strdup(str ? str : ""))) == 0 );
    g_free(text);

    if (!exists) {
        msg = N_("{not found}");
        if (! (fmt_opts & FMT_OPT_NO_TRANSLATE) )
            msg = _(msg);
        if (fmt_opts & FMT_OPT_NULL_IF_MISSING)
            return NULL;
        if (fmt_opts & FMT_OPT_ATERM)
            return g_strdup_printf( ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, msg );
        return g_strdup( msg );
    }
    if (fail) {
        msg = N_("{permission denied}");
        if (! (fmt_opts & FMT_OPT_NO_TRANSLATE) )
            msg = _(msg);
        if (fmt_opts & FMT_OPT_NULL_IF_EMPTY)
            return NULL;
        if (fmt_opts & FMT_OPT_NULL_IF_MISSING)
            return NULL;
        if (fmt_opts & FMT_OPT_ATERM)
            return g_strdup_printf( ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, msg );
        return g_strdup( msg );
    }
    if (dir) {
        msg = N_("{node}");
        if (! (fmt_opts & FMT_OPT_NO_TRANSLATE) )
            msg = _(msg);
        if (fmt_opts & FMT_OPT_NULL_IF_SIMPLE_DIR)
            return NULL;
        if (fmt_opts & FMT_OPT_ATERM)
            return g_strdup_printf( ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, msg );
        return g_strdup( msg );
    }
    if (empty) {
        msg = N_("{empty}");
        if (! (fmt_opts & FMT_OPT_NO_TRANSLATE) )
            msg = _(msg);
        if (fmt_opts & FMT_OPT_NULL_IF_EMPTY)
            return NULL;
        if (fmt_opts & FMT_OPT_ATERM)
            return g_strdup_printf( ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, msg );
        return g_strdup( msg );
    }
    if (!utf8) {
        msg = N_("{binary value}");
        if (! (fmt_opts & FMT_OPT_NO_TRANSLATE) )
            msg = _(msg);
        if (fmt_opts & FMT_OPT_ATERM)
            return g_strdup_printf( ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, msg );
        return g_strdup( msg );
    }

    /* a string */
    if (fmt_opts & FMT_OPT_LIST_ITEM &&
        (obj->data.lines > 1 || obj->data.len > 120) )
        text = g_strdup_printf(_("{%lu line(s) text, utf8}"), obj->data.lines);
    else
        text = g_strdup(str);
    g_strchomp(text);

    if (fmt_opts & FMT_OPT_HTML
        || fmt_opts & FMT_OPT_PANGO) {
        nice = util_escape_markup(text, FALSE);
        g_free(text);
    } else
        nice = text;

    return nice;
}

const sysobj_class *class_add(sysobj_class *c) {
    if (c) {
        if (g_slist_find(class_list, c) ) {
            DEBUG("Duplicate class address: 0x%llx (%s)", (long long unsigned)c, c->tag ? c->tag : "untagged");
            return NULL;
        }
        if (class_has_flag(c, OF_BLAST) )
            class_list = g_slist_append(class_list, c);
        else
            class_list = g_slist_prepend(class_list, c);
    }
    return c;
}

const sysobj_class *class_add_simple(const gchar *pattern, const gchar *label, const gchar *tag, guint flags, double update_interval, attr_tab *attributes) {
    sysobj_class *nc = class_new();
    nc->tag = tag;
    nc->pattern = pattern;
    nc->flags = flags;
    nc->s_label = label;
    nc->s_update_interval = update_interval;
    nc->attributes = attributes;
    return class_add(nc);
}

gboolean sysobj_has_flag(sysobj *s, guint flag) {
    if (s) {
        const sysobj_class *c = s->cls;
        if (c) {
            guint f = c->flags;
            if (c->f_flags)
                f = c->f_flags(s, c);
            else if (c->attributes) {
                int i = attr_tab_lookup(c->attributes, s->name);
                if (i != -1)
                    f |= c->attributes[i].extra_flags;
            }
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
            f = c->f_flags(NULL, c);
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
    sysobj_stats.so_new++;
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
        d->any = NULL;
        g_slist_free_full(d->childs, (GDestroyNotify)g_free);
        d->childs = NULL;
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
        sysobj_stats.so_clean++;
    }
}

void sysobj_free(sysobj *s) {
    if (s) {
        sysobj_free_and_clean(s);
        g_free(s);
        sysobj_stats.so_free++;
    }
}

void sysobj_classify(sysobj *s) {
    GSList *l = NULL;
    sysobj_class *c = NULL, *c_blast = NULL;
    gsize len = 0;
    if (s) {
        len = strlen(s->path);
        for (l = class_list; l; l = l->next) {
            c = l->data;
            gboolean match = FALSE;

            if ( class_has_flag(c, OF_GLOB_PATTERN) ) {
                if (!c->pspec)
                    c->pspec = g_pattern_spec_new(c->pattern);
                match = g_pattern_match(c->pspec, len, s->path, NULL);
            } else
                match = g_str_has_suffix(s->path, c->pattern);

            if (match && c->f_verify)
                match = c->f_verify(s);
            else if (match && c->attributes) {
                int i = attr_tab_lookup(c->attributes, s->name);
                match = (i != -1) ? TRUE : FALSE;
            }

            if (match) {
                if (class_has_flag(c, OF_BLAST) ) {
                    if (!c_blast) c_blast = c;
                } else {
                    c->hits++;
                    s->cls = c;
                    return;
                }
            }
        }
        if (c_blast) {
            c_blast->hits++;
            s->cls = c_blast;
            return;
        }
        sysobj_stats.classify_none++;
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
                    if (fst.st_mode & S_IRUSR) s->root_can_read = TRUE;
                    if (fst.st_mode & S_IWUSR) s->root_can_write = TRUE;
                    if (fst.st_mode & S_IROTH) s->others_can_read = TRUE;
                    if (fst.st_mode & S_IWOTH) s->others_can_write = TRUE;
                }
            }
        }
    }
}

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
    sysobj_data_free(&s->data, FALSE);
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
                sysobj_data_free(&s->data, FALSE);
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
        if (0) {
            sysobj_data_free(&s->data, FALSE);
            s->data.was_read =
                g_file_get_contents(s->path_fs, &s->data.str, &s->data.len, &error);
                if (!s->data.str) {
                    if (error && error->code == G_FILE_ERROR_ACCES)
                        s->access_fail = TRUE;
                }
                if (error)
                    g_error_free(error);
        } else {
            sysobj_data_free(&s->data, FALSE);
            int err = 0;
            s->data.was_read = gg_file_get_contents_non_blocking(s->path_fs, &s->data.str, &s->data.len, &err);
            if (!s->data.str) {
                if (err == EACCES)
                    s->access_fail = TRUE;
            }
        }
    }

    if (s->data.was_read) {
        s->data.is_utf8 = g_utf8_validate(s->data.str, s->data.len, NULL);
        if (s->data.is_utf8) {
            s->data.lines = util_count_lines(s->data.str);
            s->data.maybe_num = util_maybe_num(s->data.str);
        }
    }
}

gboolean sysobj_read_(sysobj *s, gboolean force, const char *call_func) {
    //printf("sysobj_read(%s, %s) from %s() fast:%s\n", s->path_req, force ? "true" : "false", call_func, s->fast_mode ? "true" : "false");
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
        if (s->cls) {
            if (s->cls->f_update_interval)
                return s->cls->f_update_interval(s);
            if (s->cls->s_update_interval)
                return s->cls->s_update_interval;
            if (s->cls->attributes) {
                int i = attr_tab_lookup(s->cls->attributes, s->name);
                if (i != -1) {
                    double ui = s->cls->attributes[i].s_update_interval;
                    if (ui >= 0)
                        return ui;
                }
            }
        }
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

gboolean sysobj_config_paths(sysobj *s, const gchar *base, const gchar *name) {
    gchar *req = NULL, *norm = NULL, *vlink = NULL, *chkpath = NULL, *fspath = NULL, *fspath_req = NULL;
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

        if (req_is_real) {
            fspath_req = g_strdup_printf("%s%s%s", sysobj_root, (*req == '/') ? "" : "/", req);
            g_free(req);
            s->path_req_fs = fspath_req;
            s->path_req = s->path_req_fs + alt_root_len;
        }

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

sysobj *sysobj_new_fast(const gchar *path) {
    sysobj *s = NULL;
    if (path) {
        s = sysobj_new();
        s->fast_mode = TRUE;
        sysobj_config_paths(s, path, NULL);
        s->name_req = g_path_get_basename(s->path_req);
        s->name = g_path_get_basename(s->path);
        sysobj_fscheck(s);
        sysobj_stats.so_new_fast++;
    }
    return s;
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

sysobj *sysobj_new_from_printf(gchar *path_fmt, ...) {
    sysobj *ret = NULL;
    gchar *path = NULL;
    va_list args;
    va_start(args, path_fmt);
    path = g_strdup_vprintf(path_fmt, args);
    va_end(args);
    ret = sysobj_new_from_fn(path, NULL);
    g_free(path);
    return ret;
}

gchar *sysobj_parent_path_ex(const sysobj *s, gboolean req) {
    if (s) {
        gchar *pp = req
            ? g_path_get_dirname(s->path_req)
            : g_path_get_dirname(s->path);
        return pp;
    }
    return NULL;
}

gchar *sysobj_parent_name_ex(const sysobj *s, gboolean req) {
    if (s) {
        gchar *pp = sysobj_parent_path_ex(s, req);
        if (pp) {
            gchar *pn = g_path_get_basename(pp);
            g_free(pp);
            return pn;
        }
    }
    return NULL;
}

const gchar *sysobj_label(sysobj *s) {
    if (s && s->cls) {
        if (s->cls->f_label)
            return s->cls->f_label(s);
        if (s->cls->attributes) {
            int i = attr_tab_lookup(s->cls->attributes, s->name);
            if (i != -1)
                if (s->cls->attributes[i].s_label)
                    return _(s->cls->attributes[i].s_label);
        }
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
        if (s->cls)
            if (s->cls->f_format)
                return s->cls->f_format(s, fmt_opts);
            else if (s->cls->attributes) {
                int i = attr_tab_lookup(s->cls->attributes, s->name);
                if (i != -1)
                    if (s->cls->attributes[i].fmt_func)
                        return s->cls->attributes[i].fmt_func(s, fmt_opts);
            }
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
        if (SEQ(pn, parent_name))
            verified = TRUE;
        g_free(pn);
    }
    return verified;
}

gboolean verify_true(sysobj *obj) { return TRUE; }

gboolean verify_in_attr_tab(sysobj *obj, attr_tab *attributes) {
    return !(attr_tab_lookup(attributes, obj->name) == -1);
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

    sysobj_append_data_path(".");
    vendor_init();
    class_init();

    /* if there is a main loop, then this will call
     * free_auto_free() in idle time every AF_SECONDS seconds.
     * If there is no main loop, then free_auto_free()
     * will be called at sysobj_cleanup() and when exiting
     * threads, as in sysobj_foreach(). */
    free_event_source = g_timeout_add_seconds(AF_SECONDS, (GSourceFunc)free_auto_free_sf, NULL);
    sysobj_stats.auto_free_next = sysobj_elapsed() + AF_SECONDS;
}

double sysobj_elapsed() {
    return g_timer_elapsed(sysobj_global_timer, NULL);
}

void sysobj_cleanup() {
    free_auto_free_final();
    class_cleanup();
    sysobj_virt_cleanup();
    vendor_cleanup();
    g_timer_destroy(sysobj_global_timer);
    g_slist_free_full(sysobj_data_paths, (GDestroyNotify)g_free);
}

sysobj *sysobj_parent(sysobj *s, gboolean req) {
    gchar *rpp = NULL;
    sysobj *ret = NULL;

    if (s) {
        rpp = req
            ? g_path_get_dirname(s->path_req)
            : g_path_get_dirname(s->path);

        if (SEQ(rpp, ".") )
            goto sysobj_parent_none;

        if (s->fast_mode)
            ret = sysobj_new_fast(rpp);
        else
            ret = sysobj_new_from_fn(rpp, NULL);

        if (g_str_has_prefix(ret->path, ":error/") )
            goto sysobj_parent_none;

        g_free(rpp);
        return ret;
    }
sysobj_parent_none:
    sysobj_free(ret);
    g_free(rpp);
    return NULL;
}

sysobj *sysobj_child(sysobj *s, gchar *child) {
    return sysobj_new_from_fn(s->path_req, child);
}

sysobj *sysobj_sibling(sysobj *s, gchar *child_path, gboolean req) {
    if (s) {
        gchar *rpp = req
            ? g_path_get_dirname(s->path_req)
            : g_path_get_dirname(s->path);
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
        if (ui < 0)
            ui = UPDATE_INTERVAL_DEFAULT;
        if ( (sysobj_elapsed() - s->data.stamp) >= ui ) {
            return TRUE;
        }
    }
    return FALSE;
}

int class_count() {
    return g_slist_length(class_list);
}

vendor_list vendor_list_concat_va(int count, vendor_list vl, ...) {
    vendor_list ret = vl, p = NULL;
    va_list ap;
    va_start(ap, vl);
    if (count > 0) {
        count--; /* includes vl */
        while (count) {
            p = va_arg(ap, vendor_list);
            ret = g_slist_concat(ret, p);
            count--;
        }
    } else {
        p = va_arg(ap, vendor_list);
        while (p) {
            ret = g_slist_concat(ret, p);
            p = va_arg(ap, vendor_list);
        }
    }
    va_end(ap);
    return ret;
}

vendor_list vendor_list_remove_duplicates(vendor_list vl) {
    for (GSList *l = vl; l; l = l->next) {
        GSList *d = NULL;
        while(d = g_slist_find(l->next, l->data) )
            vl = g_slist_delete_link(vl, d);
    }
    return vl;
}

vendor_list sysobj_vendors(sysobj *s) {
    if (sysobj_has_flag(s, OF_IS_VENDOR) ) {
        sysobj_read(s, FALSE);
        if (s->cls && s->cls->f_vendors)
            return s->cls->f_vendors(s);
        else {
            if (!s->data.is_utf8)
                return NULL;
            const Vendor *v = vendor_match(s->data.str, NULL);
            if (v)
                return vendor_list_append(NULL, v);
        }
    }
    return NULL;
}

vendor_list sysobj_vendors_from_fn(const gchar *base, const gchar *name) {
    sysobj *t = sysobj_new_from_fn(base, name);
    vendor_list ret = sysobj_vendors(t);
    sysobj_free(t);
    return ret;
}

int attr_tab_lookup(const attr_tab *attributes, const gchar *name) {
    for(int i = 0; attributes[i].attr_name; i++)
        if (SEQ(name, attributes[i].attr_name))
            return i;
    return -1;
}

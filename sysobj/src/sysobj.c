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
#include "format_funcs.h"

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

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/.testing/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/.testing", NULL },

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
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/dma", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/ioports", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/iomem", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/driver", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/driver/*", NULL },

    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/net", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/net/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/pmu", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/pmu/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/acpi", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/acpi/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/scsi", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/scsi/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/ide", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/ide/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/omnibook", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/omnibook/*", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/xen", NULL },
    { SO_FILTER_STATIC | SO_FILTER_INCLUDE,     "/proc/xen/*", NULL },

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

gchar *sysobj_format_from_printf(int fmt_opts, gchar *path_fmt, ...) {
    gchar *path = NULL;
    va_list args;
    va_start(args, path_fmt);
    path = g_strdup_vprintf(path_fmt, args);
    va_end(args);
    gchar *ret = sysobj_format_from_fn(path, NULL, fmt_opts);
    g_free(path);
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

uint32_t sysobj_uint32_from_printf(int nbase, gchar *path_fmt, ...) {
    gchar *path = NULL;
    va_list args;
    va_start(args, path_fmt);
    path = g_strdup_vprintf(path_fmt, args);
    va_end(args);
    uint32_t ret = sysobj_uint32_from_fn(path, NULL, nbase);
    g_free(path);
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
        wo     = obj->write_only,
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
    if (wo) {
        msg = N_("{write-only}");
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
        if (obj->cls && obj->cls->s_node_format) {
            gchar *ret = format_node_fmt_str(obj, fmt_opts, obj->cls->s_node_format);
            if (ret)
                return ret;
        }
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

#define class_msg(fmt, ...) fprintf (stderr, "[%s] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

const sysobj_class *class_add(sysobj_class *c) {
    if (c) {
        if (g_slist_find(class_list, c) ) {
            class_msg("duplicate class address: %p (%s)", c, c->tag ? c->tag : "untagged");
            return NULL;
        }
        if (c->v_is_attr && c->v_is_node) {
            class_msg("invalid class requires item to be both an attribute and a node (%p; %s; %s:%d)", c, c->tag ? c->tag : "untagged", c->def_file ? c->def_file : "?", c->def_line);
            return NULL;
        }
        for(GSList *l = class_list; l; l = l->next) {
            sysobj_class *ec = l->data;
            if (SEQ(ec->tag, c->tag) ) {
                class_msg("warning: duplicate tag %s for existing %p(\"%s\",%s:%d) and new %p(\"%s\",%s:%d)",
                    ec->tag,
                    ec, ec->pattern, ec->def_file ? ec->def_file : "?", ec->def_line,
                    c, c->pattern, c->def_file ? c->def_file : "?", c->def_line );
                break;
            }
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

guint sysobj_flags(sysobj *s) {
    guint f = OF_NONE;
    if (s) {
        const sysobj_class *c = s->cls;
        if (c) {
            f = c->flags;
            if (c->f_flags)
                f = c->f_flags(s, c);
            else if (c->attributes) {
                int i = attr_tab_lookup(c->attributes, s->name);
                if (i != -1)
                    f |= c->attributes[i].extra_flags;
            }
        }
    }
    return f;
}

gboolean sysobj_has_flag(sysobj *s, guint flag) {
    guint f = sysobj_flags(s);
    if (f & flag)
        return TRUE;
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
        g_free(s->req_link_target);
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
    sysobj_class *c = NULL, *c_blast = NULL;
    gsize len = 0;
    if (s && !s->cls) {
        len = strlen(s->path);
        for (GSList *l = class_list; l; l = l->next) {
            sysobj_stats.so_class_iter++;
            c = l->data;
            gboolean match = FALSE;
            char *reason = "<none>"; /* first fail reason */

            if ( class_has_flag(c, OF_GLOB_PATTERN) ) {
                if (!c->pspec)
                    c->pspec = g_pattern_spec_new(c->pattern);
                match = g_pattern_match(c->pspec, len, s->path, NULL);
                sysobj_stats.classify_pattern_cmp++;
            } else
                match = g_str_has_suffix(s->path, c->pattern);

            if (!match) { reason = "pattern"; }

#define SIMPLE_VERIFY(SV, TEST_CODE) \
    if (match && c->SV) { match = TEST_CODE; if (!match) reason = #SV; }

            /* simple verifiers */
            SIMPLE_VERIFY(v_name, SEQ(s->name, c->v_name) );
            SIMPLE_VERIFY(v_is_node, s->data.is_dir );
            SIMPLE_VERIFY(v_is_attr, !(s->data.is_dir) );
            SIMPLE_VERIFY(v_lblnum, verify_lblnum(s, c->v_lblnum) );
            SIMPLE_VERIFY(v_lblnum_child, verify_lblnum_child(s, c->v_lblnum_child) );
            SIMPLE_VERIFY(v_subsystem, verify_subsystem(s, c->v_subsystem) );
            SIMPLE_VERIFY(v_subsystem_parent, verify_subsystem_parent(s, c->v_subsystem_parent) );
            SIMPLE_VERIFY(v_parent_path_suffix, verify_parent(s, c->v_parent_path_suffix) );
            SIMPLE_VERIFY(v_parent_class, verify_parent_class(s, c->v_parent_class) );

            /* verify function, or verify by existence in attributes */
            if (match && c->f_verify) {
                match = c->f_verify(s);
                if (!match) reason = "f_verify";
            } else if (match && c->attributes) {
                int i = attr_tab_lookup(c->attributes, s->name);
                match = (i != -1) ? TRUE : FALSE;
                if (!match) reason = "attributes";
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
        s->write_only = FALSE;
        s->data.is_dir = FALSE;

        if (*(s->path) == ':') {
            /* virtual */
            const sysobj_virt *vo = sysobj_virt_find(s->path);
            if (vo) {
                int t = sysobj_virt_get_type(vo, s->path);
                if (t == VSO_TYPE_NONE) return;
                s->exists = TRUE;
                s->root_can_read = TRUE;
                s->others_can_read = (t & VSO_TYPE_REQ_ROOT) ? FALSE : TRUE;
                s->root_can_write = (t & VSO_TYPE_WRITE_REQ_ROOT || t & VSO_TYPE_WRITE) ? TRUE : FALSE;
                s->others_can_write = (t & VSO_TYPE_WRITE) ? TRUE : FALSE;
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
        if (s->root_can_write && !s->root_can_read)
            s->write_only = TRUE;
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
        s->exists = FALSE;
        const sysobj_virt *vo = sysobj_virt_find(s->path);
        if (vo) {
            int t = sysobj_virt_get_type(vo, s->path);
            if (t == VSO_TYPE_NONE)
                return; /* if it existed, it doesn't now */
            s->exists = TRUE;

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
        sysobj_data_free(&s->data, FALSE);
        int err = 0;
        s->data.was_read = gg_file_get_contents_non_blocking(s->path_fs, &s->data.str, &s->data.len, &err);
        if (!s->data.str && err == EACCES)
            s->access_fail = TRUE;
    }

    if (s->data.was_read && s->data.str) {
        sysobj_stats.so_read_bytes += s->data.len;
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
        if (s->write_only) {
            sysobj_stats.so_read_wo++;
            return FALSE;
        }

        if (!force && s->data.was_read) {
            if (!sysobj_data_expired(s)) {
                sysobj_stats.so_read_not_expired++;
                return FALSE;
            }
            sysobj_stats.so_read_expired++;
        }

        if (!s->data.was_read) sysobj_stats.so_read_first++;
        if (force) sysobj_stats.so_read_force++;

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

/* attributes.s_update_interval > cls->f_update_interval > s_update_interval > UPDATE_INTERVAL_DEFAULT  */
double sysobj_update_interval(sysobj *s) {
    if (s) {
        if (s->cls) {
            if (s->cls->attributes) {
                int i = attr_tab_lookup(s->cls->attributes, s->name);
                if (i != -1) {
                    double ui = s->cls->attributes[i].s_update_interval;
                    if (ui != UPDATE_INTERVAL_UNSPECIFIED)
                        return ui;
                }
            }
            if (s->cls->f_update_interval) {
                double ui = s->cls->f_update_interval(s);
                if (ui != UPDATE_INTERVAL_UNSPECIFIED)
                    return ui;
            }
            if (s->cls->s_update_interval
                && s->cls->s_update_interval != UPDATE_INTERVAL_UNSPECIFIED)
                return s->cls->s_update_interval;
        }
        return UPDATE_INTERVAL_UNSPECIFIED;
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

GSList *sysobj_children(sysobj *s, const gchar *include_glob, const gchar *exclude_glob, gboolean sort) {
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
    else {
        s->req_is_link = g_file_test(s->path_req_fs, G_FILE_TEST_IS_SYMLINK);
        if (s->req_is_link) {
            gchar *lt = g_file_read_link(s->path_req_fs, NULL);
            if (lt) {
                s->req_link_target = g_filename_to_utf8(lt, -1, NULL, NULL, NULL);
                g_free(lt);
            }
        }
    }

    //DEBUG("\n{ .path_req_fs = %s\n  .path_req = %s\n  .path_fs = %s\n  .path = %s\n  .req_is_link = %s }",
    //    s->path_req_fs, s->path_req, s->path_fs, s->path, s->req_is_link ? "TRUE" : "FALSE" );

    if (target_is_real
        && !sysobj_filter_item_include(s->path, sysobj_global_filters) ) {
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

sysobj *sysobj_new_fast_from_fn(const gchar *base, const gchar *name) {
    gchar *path = util_build_fn(base, name);
    sysobj *ret = sysobj_new_fast(path);
    g_free(path);
    return ret;
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

/* attributes.fmt_func > cls->f_format > simple_format */
gchar *sysobj_format(sysobj *s, int fmt_opts) {
    if (s) {
        sysobj_read(s, FALSE);
        if (s->cls) {
            if (s->cls->attributes) {
                int i = attr_tab_lookup(s->cls->attributes, s->name);
                if (i != -1)
                    if (s->cls->attributes[i].fmt_func)
                        return s->cls->attributes[i].fmt_func(s, fmt_opts);
            }
            if (s->cls->f_format)
                return s->cls->f_format(s, fmt_opts);
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

gboolean verify_parent_class(sysobj *obj, const gchar *tag) {
    gboolean ret = FALSE;
    sysobj *pobj = sysobj_parent(obj, FALSE);
    sysobj_classify(pobj); /* may be fast_mode */
    if (pobj->cls)
        if (SEQ(pobj->cls->tag, tag) )
            ret = TRUE;
    sysobj_free(pobj);
    return ret;
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

gboolean verify_subsystem(sysobj *obj, const gchar *target) {
    gboolean ret = FALSE;
    gchar *ssl = util_build_fn(obj->path, "subsystem");
    sysobj *sso = sysobj_new_fast(ssl);
    if (sso->exists && SEQ(sso->path, target))
        ret = TRUE;
    else {
        /* sometimes the snapshot doesn't include all of /sys, but
         * it still may be possible to match by looking at the link target */
        if (sso->req_link_target
            && g_str_has_prefix(target, "/sys")
            && g_str_has_suffix(sso->req_link_target, target + 4) ) {
            return TRUE;
        }
    }
    sysobj_free(sso);
    return ret;
}

gboolean verify_subsystem_parent(sysobj *obj, const gchar *target) {
    gboolean ret = FALSE;
    gchar *ssl = util_build_fn(obj->path, "../subsystem");
    sysobj *sso = sysobj_new_fast(ssl);
    if (sso->exists && SEQ(sso->path, target))
        ret = TRUE;
    else {
        /* sometimes the snapshot doesn't include all of /sys, but
         * it still may be possible to match by looking at the link target */
        if (sso->req_link_target
            && g_str_has_prefix(target, "/sys")
            && g_str_has_suffix(sso->req_link_target, target + 4) ) {
            return TRUE;
        }
    }
    sysobj_free(sso);
    return ret;
}

void sysobj_init(const gchar *alt_root) {
    if (alt_root)
        sysobj_root_set(alt_root);

    int i = 0;
    while(path_filters[i].type != SO_FILTER_NONE) {
        sysobj_global_filters = g_slist_append(sysobj_global_filters, &path_filters[i]);
        i++;
    }

    if (!alt_root) {
        gchar *self_net = util_canonicalize_path("/proc/net");
        if (self_net) {
            gchar *self_net_kids = g_strdup_printf("%s/*", self_net);
            sysobj_global_filters = g_slist_append(sysobj_global_filters,
                sysobj_filter_new(SO_FILTER_INCLUDE, self_net) );
            sysobj_global_filters = g_slist_append(sysobj_global_filters,
                sysobj_filter_new(SO_FILTER_INCLUDE, self_net_kids) );
            g_free(self_net_kids);
        }
        g_free(self_net);
    }

    sysobj_global_timer = g_timer_new();
    g_timer_start(sysobj_global_timer);

    sysobj_append_data_path(".");
    sysobj_virt_init();
    vendor_init();
    class_init();
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
        if (ui == UPDATE_INTERVAL_DEFAULT
            || ui == UPDATE_INTERVAL_UNSPECIFIED
            || ui <= 0 )
            ui = UPDATE_INTERVAL_DEFAULT_VALUE;
        if ( (sysobj_elapsed() - s->data.stamp) >= ui ) {
            return TRUE;
        }
    }
    return FALSE;
}

int class_count() {
    return g_slist_length(class_list);
}

vendor_list simple_vendors(sysobj *s) {
    if (sysobj_has_flag(s, OF_HAS_VENDOR) ) {
        sysobj_read(s, FALSE);
        if (!s->data.is_utf8)
            return NULL;
        //const Vendor *v = vendor_match(s->data.str, NULL);
        //if (v)
        //    return vendor_list_append(NULL, v);
        return vendors_match(s->data.str, NULL);
    }
    return NULL;
}

vendor_list sysobj_vendors(sysobj *s) {
    vendor_list vl = NULL;
    if (s->cls && sysobj_has_flag(s, OF_HAS_VENDOR) ) {
        if (s->cls->s_vendors_from_child) {
            vl = vendor_list_concat(vl, sysobj_vendors_from_fn(s->path, s->cls->s_vendors_from_child) );
        }
        if (s->cls->f_vendors) {
            sysobj_read(s, FALSE);
            vl = vendor_list_concat(vl, s->cls->f_vendors(s));
        }
    }
    if (!vl)
        vl = simple_vendors(s);
    return gg_slist_remove_null(vl);
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

gchar *flags_str(int flags) {
#define flags_str_chk(fo) if (flags & fo) { flags_list = appf(flags_list, " | ", "%s", #fo); fchk |= fo; }
    int fchk = OF_NONE;
    gchar *flags_list = NULL;
    gchar *ret = NULL;
    flags_str_chk(OF_CONST);
    flags_str_chk(OF_BLAST);
    flags_str_chk(OF_GLOB_PATTERN);
    flags_str_chk(OF_REQ_ROOT);
    flags_str_chk(OF_HAS_VENDOR);
    if (fchk != flags)
        flags_list = appf(flags_list, " | ", "?");
    if (flags_list) {
        ret = g_strdup_printf("[%x] %s", flags, flags_list);
        g_free(flags_list);
    } else {
        if (flags == OF_NONE)
            ret = g_strdup_printf("[%x] OF_NONE", flags);
    }
    return ret;
}

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

#ifndef _SYSOBJ_H_
#define _SYSOBJ_H_

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <stdint.h>  /* for *int*_t types */
#include <ctype.h>   /* for isxdigit(), etc. */
#include "config.h"
#include "gettext.h"
#include "term_color.h" /* used in formatting output */
#include "util_sysobj.h"

#define UPDATE_INTERVAL_DEFAULT 10.0   /* in seconds */
#define UPDATE_INTERVAL_NEVER    0.0   /* in seconds */

enum {
    OF_NONE          = 0,
    OF_GLOB_PATTERN  = 1,
    OF_CONST         = 1<<1,
    /* used in "simple" classes */
    OF_REQ_ROOT      = 1<<16,
    OF_IS_VENDOR     = 1<<17,
};

enum {
    FMT_OPT_NONE      = 0,          /* plain */
    FMT_OPT_PART      = 1,          /* part of larger string */
    FMT_OPT_SHORT     = 1<<1,       /* shortest version please */
    FMT_OPT_LIST_ITEM = 1<<2,       /* listing, one line value or single line summary of larger data */
    FMT_OPT_NO_UNIT   = 1<<3,       /* don't include unit */
    FMT_OPT_NO_JUNK   = 1<<4,       /* class specific, for example: dmi/id ignores placeholder strings */
    FMT_OPT_NO_TRANSLATE = 1<<5,   /* don't translate via gettext() */

    FMT_OPT_NULL_IF_EMPTY   = 1<<8,
    FMT_OPT_NULL_IF_MISSING = 1<<9,

    FMT_OPT_ATERM  = 1<<16,  /* ANSI color terminal */
    FMT_OPT_PANGO  = 1<<17,  /* pango markup for gtk */
    FMT_OPT_HTML   = 1<<18,  /* html */
};

#define FMT_OPT_OR_NULL      FMT_OPT_NULL_IF_EMPTY | FMT_OPT_NULL_IF_MISSING
#define FMT_OPT_RAW_OR_NULL  FMT_OPT_NO_TRANSLATE | FMT_OPT_PART | FMT_OPT_NO_UNIT | FMT_OPT_OR_NULL

/* can be used in class f_format() functions that
 * don't use fmt_ops to quiet -Wunused-parameter nagging.  */
#define FMT_OPTS_IGNORE() { PARAM_NOT_UNUSED(fmt_opts); }

typedef struct sysobj sysobj;
typedef struct sysobj_data sysobj_data;

/* "halp" is like help. Reference text or
 * explaination for an object.
 * It is always markup text: the Pango-safe subset
 * of HTML, plus links. */

typedef struct sysobj_class {
    const gchar *tag;
    const gchar *pattern;
    guint flags;
    const gchar *s_label; /* label for "simple" classes, not translated */
    const gchar *s_halp;  /* markup text. halp for "simple" classes */

    gboolean (*f_verify) (sysobj *obj);      /* verify the object is of this class */
    const gchar *(*f_label)  (sysobj *obj);  /* translated label */
    gchar *(*f_format) (sysobj *obj, int fmt_opts);  /* translated human-readable value */
    double (*f_update_interval) (sysobj *obj); /* time until the value might change, in seconds */
    int (*f_compare) (const sysobj_data *a, const sysobj_data *b);
    guint (*f_flags) (sysobj *obj); /* provide flags, result replaces flags */
    void (*f_cleanup) (void); /* shutdown/cleanup function */
    const gchar *(*f_halp) (sysobj *obj); /* markup text */
} sysobj_class;

enum {
                               /*  match | no-match */
    SO_FILTER_NONE        = 0, /*     NC | NC       */
    SO_FILTER_EXCLUDE     = 1, /*      E | NC       */
    SO_FILTER_INCLUDE     = 2, /*      I | NC       */
    SO_FILTER_EXCLUDE_IIF = 5, /*      E | I        */
    SO_FILTER_INCLUDE_IIF = 6, /*      I | E        */

    SO_FILTER_IIF         = 4,
    SO_FILTER_MASK        = 7,
    SO_FILTER_STATIC      = 256, /* free the pspec, but not the filter */
};

typedef struct sysobj_filter {
    int type;
    gchar *pattern;
    GPatternSpec *pspec;
} sysobj_filter;

sysobj_filter *sysobj_filter_new(int type, gchar *pattern);
void sysobj_filter_free(sysobj_filter *f);
gboolean sysobj_filter_item_include(gchar *item, GSList *filters);
/* returns the new head of now-filtered items */
GSList *sysobj_filter_list(GSList *items, GSList *filters);

typedef struct sysobj_data {
    gboolean was_read;
    gsize len;    /* bytes */
    gsize lines;  /* 1 + newlines (-1 if the last line empty) */
    union {
        void *any;
        gchar *str;
        int32_t *int32;
        int64_t *int64;
        uint32_t *uint32;
        uint64_t *uint64;
    };
    gboolean is_utf8;
    int maybe_num; /* looks like it might be a number, value is the base (10 or 16) */
    double stamp;  /* used by pin */
} sysobj_data;

struct sysobj {
    gchar *path_req_fs;  /* includes sysobj_root */
    gchar *path_req; /* requested, points into path_req_fs */
    gchar *name_req;

    gchar *path_fs;  /* includes sysobj_root */
    gchar *path;     /* canonical, points into path_fs */
    gchar *name;

    gboolean req_is_link;
    gboolean req_was_redirected;

    gboolean exists;
    gboolean is_dir;
    gboolean access_fail; /* needed root, but didn't have it */
    sysobj_data data;
    const sysobj_class *cls;
};

enum {
    VSO_TYPE_NONE     = 0, /* an error */
    VSO_TYPE_DIR      = 1,
    VSO_TYPE_STRING   = 1<<1,

    VSO_TYPE_SYMLINK  = 1<<16, /* note: sysobj_virt symlink to sysobj_virt symlink is not supported */
    VSO_TYPE_DYN      = 1<<17, /* any path beyond */
    VSO_TYPE_AUTOLINK = 1<<18,
    VSO_TYPE_CONST    = 1<<30, /* don't free */
};

typedef struct sysobj_virt {
    gchar *path;
    int type;
    gchar *str; /* default f_get_data() uses str; */
    gchar *(*f_get_data)(const gchar *path);
    int (*f_get_type)(const gchar *path);
} sysobj_virt;

/* NULL will not change the root
 * if sysobj_root_set() was already used. */
void sysobj_init(const gchar *alt_root);
void sysobj_cleanup();

#define sysobj_virt_new() g_new0(sysobj_virt, 1)
void sysobj_virt_free(sysobj_virt *s);
gboolean sysobj_virt_add(sysobj_virt *vo); /* TRUE if added, FALSE if exists (was overwritten) or error */
gboolean sysobj_virt_add_simple(const gchar *base, const gchar *name, const gchar *data, int type);
void sysobj_virt_remove(gchar *glob);
sysobj_virt *sysobj_virt_find(const gchar *path);
gchar *sysobj_virt_get_data(const sysobj_virt *vo, const gchar *req);
int sysobj_virt_get_type(const sysobj_virt *vo, const gchar *req);
GSList *sysobj_virt_children_auto(const sysobj_virt *vo, const gchar *req);
GSList *sysobj_virt_children(const sysobj_virt *vo, const gchar *req);
GSList *sysobj_virt_all_paths();
void sysobj_virt_cleanup();

/* using the glib key-value file parser, create a tree of
 * base/group/name=value virtual sysobj's. Items before a first
 * group are put in base/name=value. */
void sysobj_virt_from_kv(gchar *base, const gchar *kv_data_in);

extern gchar sysobj_root[];
gboolean sysobj_root_set(const gchar *alt_root);

gboolean util_have_root();
void util_null_trailing_slash(gchar *str); /* in-place */
void util_strstrip_double_quotes_dumb(gchar *str); /* in-place, strips any double-quotes from the start and end of str */
gchar *util_canonicalize_path(const gchar *path);
gchar *util_normalize_path(const gchar *path, const gchar *relto);
gsize util_count_lines(const gchar *str);
gchar *util_escape_markup(gchar *v, gboolean replacing);
int32_t util_get_did(gchar *str, const gchar *lbl); /* ("cpu6", "cpu") -> 6 */

/* to be called by sysobj_class::f_verify */
gboolean verify_lblnum(sysobj *obj, const gchar *lbl);
gboolean verify_lblnum_child(sysobj *obj, const gchar *lbl);
gboolean verify_parent_name(sysobj *obj, const gchar *parent_name);
gboolean verify_parent(sysobj *obj, const gchar *parent_path_suffix);

/* to be used by sysobj_class::f_compare */
typedef int (f_compare_sysobj_data)(const sysobj_data *a, const sysobj_data *b);
int compare_str_base10(const sysobj_data *a, const sysobj_data *b);
int compare_str_base16(const sysobj_data *a, const sysobj_data *b);

#define PARAM_NOT_UNUSED(p); { p = p; }
/* can be used in class f_format() functions that
 * don't use fmt_ops to quiet -Wunused-parameter nagging.  */
#define FMT_OPTS_IGNORE() { PARAM_NOT_UNUSED(fmt_opts); }

void class_init();
GSList *class_get_list();
#define class_new() g_new0(sysobj_class, 1)
void class_free(sysobj_class *c);
const sysobj_class *class_add(sysobj_class *c);
const sysobj_class *class_add_full(sysobj_class *base,
    const gchar *tag, const gchar *pattern, const gchar *s_label, const gchar *s_halp, guint flags,
    void *f_verify, void *f_label, void *f_halp,
    void *f_format, void *f_update_interval, void *f_compare, void *f_flags );
const sysobj_class *class_add_simple(const gchar *pattern, const gchar *label, const gchar *tag, guint flags);
gboolean class_has_flag(const sysobj_class *c, guint flag);
void class_cleanup();

const gchar *simple_label(sysobj* obj);
const gchar *simple_halp(sysobj* obj);
gchar *simple_format(sysobj* obj, int fmt_opts);

sysobj *sysobj_new();
sysobj *sysobj_new_from_fn(const gchar *base, const gchar *name);
void sysobj_fscheck(sysobj *s);
void sysobj_classify(sysobj *s);
gboolean sysobj_exists(sysobj *s);
gboolean sysobj_exists_from_fn(const gchar *base, const gchar *name);
gboolean sysobj_has_flag(sysobj *s, guint flag);
void sysobj_read_data(sysobj *s);
void sysobj_unread_data(sysobj *s); /* frees data, but keeps is_utf8, len, lines, etc. */
const gchar *sysobj_label(sysobj *s);
const gchar *sysobj_halp(sysobj *s);
gchar *sysobj_format(sysobj *s, int fmt_opts);
gchar *sysobj_raw_from_fn(const gchar *base, const gchar *name);
uint32_t sysobj_uint32_from_fn(const gchar *base, const gchar *name, int nbase);
gchar *sysobj_format_from_fn(const gchar *base, const gchar *name, int fmt_opts);
double sysobj_update_interval(sysobj *s);
void sysobj_free(sysobj *s);

/* using the request parent */
sysobj *sysobj_parent(sysobj *s);
sysobj *sysobj_child(sysobj *s, gchar *child);
sysobj *sysobj_child_of_parent(sysobj *s, gchar *child_path);
/* using the real parent */
gchar *sysobj_parent_path(sysobj *s);
gchar *sysobj_parent_name(sysobj *s);
GSList *sysobj_children(sysobj *s, gchar *include_glob, gchar *exclude_glob, gboolean sort);

/* filters is list of sysobj_filter */
GSList *sysobj_children_ex(sysobj *s, GSList *filters, gboolean sort);

sysobj_data *sysobj_data_dup(sysobj_data *d);

/* table is null-terminated string list of null-terminated UNTRANSLATED strings
 * table_len, optional, improves speed
 * nbase is the base of the number stored in obj->data.str
 * fmt_opts, as usual */
gchar *sysobj_format_table(sysobj *obj, gchar **table, int table_len, int nbase, int fmt_opts);

/* adds a symlink to the :/watchlist path */
void sysobj_watchlist_add(const gchar *group, const gchar *target_base, const gchar *target_name);
void sysobj_watchlist_clear(const gchar *group);

/* debugging stuff */
void class_dump_list();

#endif

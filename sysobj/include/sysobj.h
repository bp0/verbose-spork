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
#include "sysobj_config.h"
#include "gettext.h"
#include "term_color.h" /* used in formatting output */
#include "util_sysobj.h"
#include "sysobj_virt.h"
#include "sysobj_filter.h"
#include "vendor.h"

#define UPDATE_INTERVAL_DEFAULT  10.0   /* in seconds */
#define UPDATE_INTERVAL_NEVER     0.0   /* in seconds */

enum {
    OF_NONE          = 0,
    OF_GLOB_PATTERN  = 1,
    OF_CONST         = 1<<1,
    OF_BLAST         = 1<<2,  /* match as late as possible */

    /* used in "simple" classes and attr_tabs
     * use sysobj_has_flag() to check */
    OF_REQ_ROOT      = 1<<16, /* expected to require root */
    OF_IS_VENDOR     = 1<<17, /* additional vendor information may be available through sysobj_vendor() */
                              /* TODO: rename to OF_HAS_VENDOR */

};

enum {
    FMT_OPT_NONE      = 0,          /* plain */
    FMT_OPT_PART      = 1,          /* part of larger string */
    FMT_OPT_SHORT     = 1<<1,       /* shortest version please */
    FMT_OPT_COMPLETE  = 1<<2,       /* most complete/verbose version please */
    FMT_OPT_LIST_ITEM = 1<<3,       /* listing, one line value or single line summary of larger data */
    FMT_OPT_NO_UNIT   = 1<<4,       /* don't include unit */
    FMT_OPT_NO_JUNK   = 1<<5,       /* class specific, for example: dmi/id ignores placeholder strings */
    FMT_OPT_NO_TRANSLATE = 1<<6,    /* don't translate via gettext() */

    FMT_OPT_NULL_IF_EMPTY       = 1<<8,   /* null if simple_format would return "{empty}" */
    FMT_OPT_NULL_IF_MISSING     = 1<<9,   /* null if simple_format would return "{not found}" */
    FMT_OPT_NULL_IF_SIMPLE_DIR  = 1<<10,  /* null if simple_format would return "{node}" */

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
typedef struct sysobj_class sysobj_class;

typedef gchar* (*func_format)(sysobj *obj, int fmt_opts);
typedef int (*func_compare_sysobj_data)(const sysobj_data *a, const sysobj_data *b);
typedef gboolean (*func_verify)(sysobj *obj);
typedef guint (*func_class_flags)(sysobj *obj, const sysobj_class *cls); /* remember to handle obj == NULL */
typedef GSList* vendor_list;
#define vendor_list_append(vl, v) g_slist_append(vl, (Vendor*)v)
#define vendor_list_concat(vl, ext) g_slist_concat(vl, ext)
#define vendor_list_free(vl) g_slist_free(vl)
vendor_list vendor_list_remove_duplicates(vendor_list vl);
typedef vendor_list (*func_get_vendors)(sysobj *obj);

typedef const struct {
    const gchar *attr_name;
    const gchar *s_label; /* N_() */
    int extra_flags;
    func_format fmt_func;
    double s_update_interval; /* -1 for UPDATE_INTERVAL_DEFAULT */
} attr_tab;

#define ATTR_TAB_LAST { NULL, NULL, 0, NULL, -1 }

int attr_tab_lookup(const attr_tab *attributes, const gchar *name);

/* can be used in a struct sysobj_class to provide debug information
 * sysobj_class my_class = { SYSOBJ_CLASS_DEF .pattern = "...", ... }
 * The define can be removed for "reproducible" builds. */
#define SYSOBJ_CLASS_DEF .def_file = __FILE__ + sizeof(SYSOB_SRC_ROOT), .def_line = __LINE__,

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
    const gchar *s_suggest; /* suggest an alternate path */
    double s_update_interval;

    /* will provide any of f_verify, f_format, f_label, f_flags
     * that are not already provided */
    attr_tab *attributes;

    func_verify f_verify;  /* verify the object is of this class */
    func_format f_format;  /* translated human-readable value */
    func_compare_sysobj_data f_compare;
    func_class_flags f_flags; /* provide flags, result replaces flags */
    func_get_vendors f_vendors; /* called if if OF_IS_VENDOR, default vendor_match()'s the data string */

    const gchar *(*f_label)  (sysobj *obj);  /* translated label */
    double (*f_update_interval) (sysobj *obj); /* time until the value might change, in seconds */
    void (*f_cleanup) (void); /* shutdown/cleanup function */
    const gchar *(*f_halp) (sysobj *obj); /* markup text */

    /* use SYSOBJ_CLASS_DEF for these */
    const gchar *def_file; /* __FILE__ */
    const int    def_line; /* __LINE__ */

    GPatternSpec *pspec;
    long long unsigned hits;
} sysobj_class;

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
    double stamp;  /* time last read, relative to sysobj_init() */

    gboolean is_dir;
    GSList *childs;
} sysobj_data;

struct sysobj {
    gchar *path_req_fs;  /* includes sysobj_root */
    gchar *path_req; /* requested, points into path_req_fs */
    gchar *name_req;

    gchar *path_fs;  /* includes sysobj_root */
    gchar *path;     /* canonical, points into path_fs */
    gchar *name;

    gboolean exists;
    gboolean req_is_link;
    gboolean root_can_read;
    gboolean root_can_write;
    gboolean others_can_read;
    gboolean others_can_write;
    gboolean access_fail; /* permission denied */

    gboolean fast_mode;

    sysobj_data data;
    const sysobj_class *cls;
};

/* list of paths to search for data files,
 * in search order.
 * Add paths *before* sysobj_init().
 * All strings will be freed with g_free()
 * at sysobj_cleanup() */
extern GSList *sysobj_data_paths;
#define sysobj_prepend_data_path(p) (sysobj_data_paths = g_slist_prepend(sysobj_data_paths, g_strdup(p)))
#define sysobj_append_data_path(p) (sysobj_data_paths = g_slist_append(sysobj_data_paths, g_strdup(p)))
/* returns a path including the file name, g_free() when done */
gchar *sysobj_find_data_file(const gchar *file);

gboolean sysobj_root_set(const gchar *alt_root);
const gchar *sysobj_root_get();
#define sysobj_using_alt_root() (strlen(sysobj_root_get())!=0)
/* NULL will not change the root
 * if sysobj_root_set() was already used. */
void sysobj_init(const gchar *alt_root);
void sysobj_cleanup();
double sysobj_elapsed(); /* time since sysobj_init(), in seconds */

/* to be called by sysobj_class::f_verify */
gboolean verify_true(sysobj *obj); /* return TRUE; */
gboolean verify_lblnum(sysobj *obj, const gchar *lbl);
gboolean verify_lblnum_child(sysobj *obj, const gchar *lbl);
gboolean verify_parent_name(sysobj *obj, const gchar *parent_name);
gboolean verify_parent(sysobj *obj, const gchar *parent_path_suffix);
gboolean verify_in_attr_tab(sysobj *obj, attr_tab *attributes);

/* to be used by sysobj_class::f_compare */
int compare_str_base10(const sysobj_data *a, const sysobj_data *b);
int compare_str_base16(const sysobj_data *a, const sysobj_data *b);

void class_init();
GSList *class_get_list();
int class_count();
#define class_new() g_new0(sysobj_class, 1)
void class_free(sysobj_class *c);
const sysobj_class *class_add(sysobj_class *c);
const sysobj_class *class_add_simple(const gchar *pattern, const gchar *label, const gchar *tag, guint flags, double update_interval, attr_tab *attributes);
gboolean class_has_flag(const sysobj_class *c, guint flag);
void class_cleanup();

const gchar *simple_label(sysobj* obj);
const gchar *simple_halp(sysobj* obj);
gchar *simple_format(sysobj* obj, int fmt_opts);

sysobj *sysobj_new();
sysobj *sysobj_new_fast(const gchar *path);  /* does not classify() */
sysobj *sysobj_new_from_fn(const gchar *base, const gchar *name);
sysobj *sysobj_new_from_printf(gchar *path_fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
sysobj *sysobj_dup(const sysobj *src);
void sysobj_fscheck(sysobj *s);
void sysobj_classify(sysobj *s);
gboolean sysobj_exists(sysobj *s);
gboolean sysobj_exists_from_fn(const gchar *base, const gchar *name);
gboolean sysobj_has_flag(sysobj *s, guint flag);
#define sysobj_read(o, f) sysobj_read_(o, f, __FUNCTION__);
gboolean sysobj_read_(sysobj *s, gboolean force, const char *call_func); /* TRUE = data state updated, FALSE = data state not updated. Use data.was_read to see check for read error. */
gboolean sysobj_data_expired(sysobj *s);
void sysobj_unread_data(sysobj *s); /* frees data, but keeps is_utf8, len, lines, etc. */
const gchar *sysobj_label(sysobj *s);
const gchar *sysobj_halp(sysobj *s);
const gchar *sysobj_suggest(sysobj *s);
vendor_list sysobj_vendors(sysobj *s);
vendor_list sysobj_vendors_from_fn(const gchar *base, const gchar *name);
gchar *sysobj_format(sysobj *s, int fmt_opts);
gchar *sysobj_raw_from_fn(const gchar *base, const gchar *name);
gchar *sysobj_raw_from_printf(gchar *path_fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
uint32_t sysobj_uint32_from_fn(const gchar *base, const gchar *name, int nbase);
gchar *sysobj_format_from_fn(const gchar *base, const gchar *name, int fmt_opts);
double sysobj_update_interval(sysobj *s);
void sysobj_free(sysobj *s);

#define sysobj_parent_path(s) sysobj_parent_path_ex(s, FALSE)
#define sysobj_parent_name(s) sysobj_parent_name_ex(s, FALSE)
/* req = parent in the request path */
gchar *sysobj_parent_path_ex(const sysobj *s, gboolean req);
gchar *sysobj_parent_name_ex(const sysobj *s, gboolean req);
sysobj *sysobj_parent(sysobj *s, gboolean req);
sysobj *sysobj_sibling(sysobj *s, gchar *sib_name, gboolean req);
sysobj *sysobj_child(sysobj *s, gchar *child);
GSList *sysobj_children(sysobj *s, gchar *include_glob, gchar *exclude_glob, gboolean sort);
/* filters is list of sysobj_filter */
GSList *sysobj_children_ex(sysobj *s, GSList *filters, gboolean sort);

sysobj_data *sysobj_data_dup(const sysobj_data *src);
void sysobj_data_free(sysobj_data *d, gboolean and_self);

#define DEBUG_AUTO_FREE 0
#define AF_SECONDS 31
#define AF_DELAY_SECONDS 20
#ifdef DEBUG_AUTO_FREE
#define auto_free(p) auto_free_(p, __FILE__, __LINE__, __FUNCTION__)
#define auto_free_ex(p, f) auto_free_ex_(p, f, __FILE__, __LINE__, __FUNCTION__)
#else
#define auto_free(p) auto_free_(p, NULL, 0, NULL)
#define auto_free_ex(p, f) auto_free_ex_(p, f, NULL, 0, NULL)
#endif
gpointer auto_free_(gpointer p, const char *file, int line, const char *func);
gpointer auto_free_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func);
void free_auto_free();

typedef struct {
    long long unsigned
        so_new,
        so_new_fast,
        so_clean,
        so_free,
        auto_freed,
        auto_free_len;
    double
        auto_free_next;
} so_stats;
extern so_stats sysobj_stats;

/* debugging stuff */
void class_dump_list();

#endif

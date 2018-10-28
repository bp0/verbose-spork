
#ifndef _SYSOBJ_H_
#define _SYSOBJ_H_

#include <glib.h>
#include <glib/gstdio.h>
#include <stdint.h>  /* for *int*_t types */
#include <unistd.h>  /* for getuid() */
#include "config.h"
#include "gettext.h"

enum {
    OF_NONE          = 0,
    OF_GLOB_PATTERN  = 1,
    /* used in "simple" classes */
    OF_REQ_ROOT      = 1<<16,
    OF_IS_VENDOR     = 1<<17,
};

enum {
    FMT_OPT_NONE     = 0,      /* plain */
    FMT_OPT_PART     = 1,      /* part of larger string */
    FMT_OPT_SHORT    = 1<<1,   /* shortest version please */
    FMT_OPT_NO_UNIT  = 1<<2,   /* don't include unit */

    FMT_OPT_TERM   = 1<<16,  /* terminal */
    FMT_OPT_PANGO  = 1<<17,  /* pango markup for gtk */
    FMT_OPT_HTML   = 1<<18,  /* html */
};

typedef struct sysobj sysobj;
typedef struct sysobj_data sysobj_data;

typedef struct sysobj_class {
    const gchar *pattern;
    guint flags;
    const gchar *s_label; /* label for "simple" classes */
    const gchar *s_info;  /* info for "simple" classes */

    gboolean (*f_verify) (sysobj *obj);      /* verify the object is of this class */
    const gchar *(*f_label)  (sysobj *obj);  /* translated label */
    const gchar *(*f_info)   (sysobj *obj);  /* translated description */
    gchar *(*f_format) (sysobj *obj, int fmt_opts);  /* translated human-readable value */
    double (*f_update_interval) (sysobj *obj); /* time until the value might change */
    int (*f_compare) (const sysobj_data *a, const sysobj_data *b);
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
} sysobj_data;

struct sysobj {
    gchar *path_req; /* requested */
    gchar *name_req;

    gchar *path;     /* canonical */
    gchar *name;

    gboolean exists;
    gboolean is_dir;
    gboolean access_fail; /* needed root, but didn't have it */
    sysobj_data data;
    const sysobj_class *cls;
};

gboolean util_have_root();
void util_null_trailing_slash(gchar *str);
gchar *util_canonicalize(const gchar *path);
gsize util_count_lines(const gchar *str);

/* to be called by sysobj_class::f_verify */
gboolean verify_lblnum(sysobj *obj, const gchar *lbl);
gboolean verify_lblnum_child(sysobj *obj, const gchar *lbl);
gboolean verify_parent_name(sysobj *obj, const gchar *parent_name);

/* to be used by sysobj_class::f_compare */
int compare_str_base10(const sysobj_data *a, const sysobj_data *b);

#define PARAM_NOT_UNUSED(p); { p = p; }
/* can be used in class f_format() functions that
 * don't use fmt_ops to quiet -Wunused-parameter nagging.  */
#define FMT_OPTS_IGNORE() { PARAM_NOT_UNUSED(fmt_opts); }

void class_init();
GSList *class_get_list();
void class_add(sysobj_class *c);
void class_add_simple(const gchar *pattern, const gchar *label, guint flags);
gboolean class_has_flag(const sysobj_class *c, guint flag);
void class_free_list(); /* should be called by class_cleanup(); */
void class_cleanup();

const gchar *simple_label(sysobj* obj);
gchar *simple_format(sysobj* obj, int fmt_opts);

sysobj *sysobj_new();
sysobj *sysobj_new_from_fn(const gchar *base, const gchar *name);
void sysobj_fscheck(sysobj *s);
void sysobj_classify(sysobj *s);
void sysobj_read_data(sysobj *s);
const gchar *sysobj_label(sysobj *s);
gchar *sysobj_format(sysobj *s, int fmt_opts);
void sysobj_free(sysobj *s);
gchar *sysobj_parent_path(sysobj *s);
gchar *sysobj_parent_name(sysobj *s);

gchar *sysobj_format_from_fn(const gchar *base, const gchar *name, int fmt_opts);

sysobj_data *sysobj_data_dup(sysobj_data *d);

/* debugging stuff */
void class_dump_list();

#endif

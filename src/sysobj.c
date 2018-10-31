
#include "sysobj.h"

gchar sysobj_root[1024] = "";
gboolean sysobj_root_set(const gchar *alt_root) {
    snprintf(sysobj_root, sizeof(sysobj_root) - 1, "%s", alt_root);
    util_null_trailing_slash(sysobj_root);
    return TRUE;
}

gboolean util_have_root() {
    return (getuid() == 0) ? TRUE : FALSE;
}

void util_null_trailing_slash(gchar *str) {
    if (str && *str) {
        if (str[strlen(str)-1] == '/' )
            str[strlen(str)-1] = 0;
    }
}

gsize util_count_lines(const gchar *str) {
    gchar **lines = NULL;
    gsize count = 0;

    if (str) {
        lines = g_strsplit(str, "\n", 0);
        count = g_strv_length(lines);
        if (count && *lines[count-1] == 0) {
            /* if the last line is empty, don't count it */
            count--;
        }
        g_strfreev(lines);
    }

    return count;
}

int compare_str_base10(const sysobj_data *a, const sysobj_data *b) {
    int64_t A = a ? strtol(a->str, NULL, 10) : 0;
    int64_t B = b ? strtol(b->str, NULL, 10) : 0;
    if (A < B) return -1;
    if (A > B) return 1;
    return 0;
}

GSList *class_list = NULL;

GSList *class_get_list() {
    return class_list;
}

void class_free_list() {
    g_slist_free_full(class_list, g_free);
}

const gchar *simple_label(sysobj* obj) {
    if (obj && obj->cls) {
        return obj->cls->s_label;
    }
    return NULL;
}

gchar *sysobj_format_from_fn(const gchar *base, const gchar *name, int fmt_opts) {
    gchar *ret = NULL;
    sysobj *obj = sysobj_new_from_fn(base, name);
    ret = sysobj_format(obj, fmt_opts);
    sysobj_free(obj);
    return ret;
}

gchar *simple_format(sysobj* obj, int fmt_opts) {
    static const gchar *special[] = {
        N_("<needs root>"),
        N_("<node>"),
        N_("<binary value>"),
    };

    FMT_OPTS_IGNORE();

    if (obj) {
        gchar *nice = NULL;
        if ( obj->access_fail )
            nice = g_strdup( special[0] );
        else if ( obj->is_dir )
            nice = g_strdup( special[1] );
        else if ( !obj->data.is_utf8 )
            nice = g_strdup( special[2] );
        else {
            nice = g_strdup( obj->data.str );
            g_strchomp(nice);
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
    const gchar *s_label, const gchar *s_info, guint flags,
    void *f_verify, void *f_label, void *f_info,
    void *f_format, void *f_update_interval, void *f_compare,
    void *f_flags ) {

    sysobj_class *nc = g_new0(sysobj_class, 1);
#define CLASS_PROVIDE_OR_INHERIT(M) nc->M = M ? M : (base ? base->M : 0 )
    CLASS_PROVIDE_OR_INHERIT(tag);
    CLASS_PROVIDE_OR_INHERIT(pattern);
    CLASS_PROVIDE_OR_INHERIT(s_label);
    CLASS_PROVIDE_OR_INHERIT(s_info);
    CLASS_PROVIDE_OR_INHERIT(flags);
    CLASS_PROVIDE_OR_INHERIT(f_verify);
    CLASS_PROVIDE_OR_INHERIT(f_label);
    CLASS_PROVIDE_OR_INHERIT(f_info);
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

const sysobj_class *class_add_simple(const gchar *pattern, const gchar *label, const gchar *tag, guint flags) {
    return class_add_full(NULL, tag, pattern, label, NULL, flags, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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

sysobj_data *sysobj_data_dup(sysobj_data *d) {
    if (d) {
        sysobj_data *nd = g_memdup(d, sizeof(sysobj_data));
        /* +1 because g_file_get_contents() always adds \n */
        nd->any = g_memdup(d->any, d->len + 1);
        return nd;
    }
    return NULL;
}

sysobj *sysobj_new() {
    sysobj *s = g_new0(sysobj, 1);
    return s;
}

/* keep the pointer, but make it like the result of sysobj_new() */
static void sysobj_free_and_clean(sysobj *s) {
    if (s) {
        g_free(s->path_req);
        g_free(s->name_req);
        g_free(s->path);
        g_free(s->name);
        g_free(s->data.any);
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
        s->exists = g_file_test(s->path, G_FILE_TEST_EXISTS);
        if (s->exists) {
            s->is_dir = g_file_test(s->path, G_FILE_TEST_IS_DIR);
        }
    }
}

gchar *util_canonicalize(const gchar *path) {
    char *resolved = realpath(path, NULL);
    gchar *ret = g_strdup(resolved); /* free with g_free() instead of free() */
    free(resolved);
    return ret;
}

void sysobj_read_data(sysobj *s) {
    GError *error = NULL;
    if (s && s->path) {
        s->data.was_read =
            g_file_get_contents(s->path, &s->data.str, &s->data.len, &error);
            if (s->data.str) {
                s->data.is_utf8 = g_utf8_validate(s->data.str, s->data.len, NULL);
                s->data.lines = util_count_lines(s->data.str);
            } else {
                if (sysobj_has_flag(s, OF_REQ_ROOT) && !util_have_root())
                    s->access_fail = TRUE;
                if (error && error->code == G_FILE_ERROR_ACCES)
                    s->access_fail = TRUE;
            }
            if (error)
                g_error_free(error);
    }
}

void sysobj_unread_data(sysobj *s) {
    if (s) {
        if (s->data.was_read && s->data.any)
            g_free(s->data.any);
        s->data.was_read = FALSE;
    }
}

sysobj *sysobj_new_from_fn(const gchar *base, const gchar *name) {
    sysobj *s = NULL;
    if (base) {
        s = sysobj_new();
        gchar *nbase = g_strdup_printf("%s%s%s",
            sysobj_root, (*base == '/') ? "" : "/", base );
        util_null_trailing_slash(nbase);
        if (name) {
            s->path_req = g_strdup_printf("%s/%s", nbase, name);
            g_free(nbase);
        } else {
            s->path_req = nbase;
        }
        s->name_req = g_path_get_basename(s->path_req);
        s->req_is_link = g_file_test(s->path_req, G_FILE_TEST_IS_SYMLINK);

        s->path = util_canonicalize(s->path_req);
        if (!s->path)
            s->path = g_strdup(s->path_req); /* the path may not exist */
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

gchar *sysobj_format(sysobj *s, int fmt_opts) {
    sysobj_read_data(s);
    if (s && s->cls && s->cls->f_format)
        return s->cls->f_format(s, fmt_opts);
    else
        return simple_format(s, fmt_opts);
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

int32_t util_get_did(gchar *str, const gchar *lbl) {
    int32_t id = -2;
    gchar tmpfmt[128] = "";
    gchar tmpchk[128] = "";
    sprintf(tmpfmt, "%s%s", lbl, "%d");
    if ( sscanf(str, tmpfmt, &id) ) {
        sprintf(tmpchk, tmpfmt, id);
        if ( strcmp(str, tmpchk) == 0 )
            return id;
    }
    return -1;
}

gboolean verify_lblnum(sysobj *obj, const gchar *lbl) {
    if (lbl && obj && obj->name) {
        if ( util_get_did(obj->name, lbl) >= 0 )
            return TRUE;
    }
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


#include "sysobj.h"

gchar sysobj_root[1024] = "";
#define USING_ALT_ROOT (*sysobj_root != 0)
gboolean sysobj_root_set(const gchar *alt_root) {
    gchar *fspath = util_canonicalize_path(alt_root);
    snprintf(sysobj_root, sizeof(sysobj_root) - 1, "%s", fspath);
    util_null_trailing_slash(sysobj_root);
    return TRUE;
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

    { SO_FILTER_NONE, "", NULL },
};
static GSList *sysobj_global_filters = NULL;

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
        return obj->cls->s_label;
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
    sysobj_read_data(obj);
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
        N_("{needs root}"),
        N_("{node}"),
        N_("{binary value}"),
        N_("{not found}"),
    };

    if (obj) {
        gchar *nice = NULL;
        if ( !obj->exists ) {
            if (fmt_opts & FMT_OPT_NULL_IF_MISSING)
                nice = NULL;
            else
                nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_RED), special[3] );
        } else if ( obj->access_fail )
            nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_RED), special[0] );
        else if ( obj->is_dir )
            nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_BLUE), special[1] );
        else if ( !obj->data.is_utf8 )
            nice = g_strdup_printf( TERM_COLOR_FMT(ANSI_COLOR_YELLOW), special[2] );
        else {
            if ( *(obj->data.str) == 0
                && ( fmt_opts & FMT_OPT_NULL_IF_MISSING) )
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
        g_free(s->path_req_fs);
        g_free(s->name_req);
        g_free(s->path_fs);
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

            //DEBUG("try [%s] %s for %s ... %s \n", class_has_flag(c, OF_GLOB_PATTERN) ? "glob" : "no-glob", c->pattern, s->path, match ? "match" : "no-match");

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
        if (*(s->path) == ':') {
            /* virtual */
            s->exists = FALSE;
            s->is_dir = FALSE;
            const sysobj_virt *vo = sysobj_virt_find(s->path);
            if (vo) {
                s->exists = TRUE;
                int t = sysobj_virt_get_type(vo, s->path);
                if (t & VSO_TYPE_DIR)
                    s->is_dir = TRUE;
            }
        } else {
            s->exists = g_file_test(s->path_fs, G_FILE_TEST_EXISTS);
            if (s->exists) {
                s->is_dir = g_file_test(s->path_fs, G_FILE_TEST_IS_DIR);
            }
        }
    }
}

/* resolve . and .., but not symlinks */
gchar *util_normalize_path(const gchar *path, const gchar *relto) {
    gchar *resolved = NULL;
#if GLIB_CHECK_VERSION(2, 58, 0)
    resolved = g_canonicalize_filename(path, relto);
#else
    /* burt's hack version */
    gchar *frt = relto ? g_strdup(relto) : NULL;
    util_null_trailing_slash(frt);
    gchar *fpath = frt
        ? g_strdup_printf("%s%s%s", frt, (*path == '/') ? "" : "/", path)
        : g_strdup(path);
    g_free(frt);

    /* note: **parts will own all the part strings throughout */
    gchar **parts = g_strsplit(fpath, "/", -1);
    gsize i, pn = g_strv_length(parts);
    GList *lparts = NULL, *l = NULL, *n = NULL, *p = NULL;
    for (i = 0; i < pn; i++)
        lparts = g_list_append(lparts, parts[i]);

    i = 0;
    gchar *part = NULL;
    l = lparts;
    while(l) {
        n = l->next; p = l->prev;
        part = l->data;

        if (!g_strcmp0(part, ".") )
            lparts = g_list_delete_link(lparts, l);

        if (!g_strcmp0(part, "..") ) {
            if (p)
                lparts = g_list_delete_link(lparts, p);
            lparts = g_list_delete_link(lparts, l);
        }

        l = n;
    }

    resolved = g_strdup("");
    l = lparts;
    while(l) {
        resolved = g_strdup_printf("%s%s/", resolved, (gchar*)l->data );
        l = l->next;
    }
    g_list_free(lparts);
    util_null_trailing_slash(resolved);
    g_free(fpath);

    g_strfreev(parts);
#endif

    return resolved;
}

/* resolve . and .. and symlinks */
gchar *util_canonicalize_path(const gchar *path) {
    char *resolved = realpath(path, NULL);
    gchar *ret = g_strdup(resolved); /* free with g_free() instead of free() */
    free(resolved);
    return ret;
}

int util_maybe_num(gchar *str) {
    int r = 10, i = 0, l = (str) ? strlen(str) : 0;
    if (!l || l > 32) return 0;
    gchar *chk = g_strdup(str);
    g_strstrip(chk);
    l = strlen(chk);
    for (i = 0; i < l; i++) {
        if (isxdigit(chk[i]))  {
            if (!isdigit(chk[i]))
                r = 16;
        } else {
            r = 0;
            break;
        }
    }
    g_free(chk);
    return r;
}

void sysobj_read_data(sysobj *s) {
    GError *error = NULL;
    if (s && s->path) {
        DEBUG("[0x%llx] %s", (long long unsigned)s, s->path);
        if (s->is_dir)
            s->data.was_read = TRUE;
        else if (*(s->path) == ':') {
            /* virtual */
            s->data.was_read = FALSE;
            const sysobj_virt *vo = sysobj_virt_find(s->path);
            if (vo) {
                s->data.str = sysobj_virt_get_data(vo, s->path);
                if (s->data.str) {
                    s->data.was_read = TRUE;
                    s->data.len = strlen(s->data.str); //TODO:
                } else {
                    if (sysobj_has_flag(s, OF_REQ_ROOT) && !util_have_root())
                        s->access_fail = TRUE;
                }
            }
        } else {
            s->data.was_read =
                g_file_get_contents(s->path_fs, &s->data.str, &s->data.len, &error);
                if (!s->data.str) {
                    if (sysobj_has_flag(s, OF_REQ_ROOT) && !util_have_root())
                        s->access_fail = TRUE;
                    if (error && error->code == G_FILE_ERROR_ACCES)
                        s->access_fail = TRUE;
                }
                if (error)
                    g_error_free(error);
        }

        s->data.is_utf8 = g_utf8_validate(s->data.str, s->data.len, NULL);
        if (s->data.is_utf8) {
            s->data.lines = util_count_lines(s->data.str);
            s->data.maybe_num = util_maybe_num(s->data.str);
        }
    }
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
        if (s->cls && s->cls->f_update_interval) {
            return s->cls->f_update_interval(s);
        }
        return UPDATE_INTERVAL_DEFAULT;
    }
    return UPDATE_INTERVAL_NEVER;
}

GSList *sysobj_children_ex(sysobj *s, GSList *filters, gboolean sort) {
    GSList *ret = NULL;
    GDir *dir;
    const gchar *fn;

    if (s) {
        if (*(s->path) == ':') {
            /* virtual */
            const sysobj_virt *vo = sysobj_virt_find(s->path);
            if (vo)
                ret = sysobj_virt_children(vo, s->path);
        } else {
            /* normal */
            dir = g_dir_open(s->path_fs, 0 , NULL);
            if (dir) {
                while((fn = g_dir_read_name(dir)) != NULL) {
                    ret = g_slist_append(ret, g_strdup(fn));
                }
                g_dir_close(dir);
            }
        }

        if (filters)
            ret = sysobj_filter_list(ret, filters);

        if (sort)
            ret = g_slist_sort(ret, (GCompareFunc)g_strcmp0);
    }
    return ret;
}

GSList *sysobj_children(sysobj *s, gchar *include_glob, gchar *exclude_glob, gboolean sort) {
    GSList *filters = NULL, *ret = NULL;
    if (include_glob)
        filters = g_slist_append(filters, sysobj_filter_new(SO_FILTER_INCLUDE_IIF, include_glob) );
    if (exclude_glob)
        filters = g_slist_append(filters, sysobj_filter_new(SO_FILTER_EXCLUDE, exclude_glob) );
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
            g_free(vlink);
            vlink = vlink_tmp;
            vlr_count++;
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
        fspath = g_strdup(req);
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

gchar *sysobj_format(sysobj *s, int fmt_opts) {
    if (s) {
        if (!s->data.was_read) {
            DEBUG("reading %s", s->path);
            sysobj_read_data(s);
        } else {
            DEBUG("already read %s", s->path);
        }
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

gchar *util_escape_markup(gchar *v, gboolean replacing) {
    gchar *clean, *tmp;
    gchar **vl;
    if (v == NULL) return NULL;

    vl = g_strsplit(v, "&", -1);
    if (g_strv_length(vl) > 1)
        clean = g_strjoinv("&amp;", vl);
    else
        clean = g_strdup(v);
    g_strfreev(vl);

    vl = g_strsplit(clean, "<", -1);
    if (g_strv_length(vl) > 1) {
        tmp = g_strjoinv("&lt;", vl);
        g_free(clean);
        clean = tmp;
    }
    g_strfreev(vl);

    vl = g_strsplit(clean, ">", -1);
    if (g_strv_length(vl) > 1) {
        tmp = g_strjoinv("&gt;", vl);
        g_free(clean);
        clean = tmp;
    }
    g_strfreev(vl);

    if (replacing)
        g_free((gpointer)v);
    return clean;
}

void sysobj_virt_add(sysobj_virt *vo) {
    if (vo) {
        vo_list = g_slist_append(vo_list, vo);
    }
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
            break;
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

void util_strstrip_double_quotes_dumb(gchar *str) {
    if (!str) return;
    g_strstrip(str);
    gchar *front = str, *back = str + strlen(str) - 1;
    while(*front == '"') { *front = 'X'; front++; }
    while(*back == '"') { *back = 0; back--; }
    int nl = strlen(front);
    memmove(str, front, nl);
    str[nl] = 0;
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

GSList *sysobj_virt_children_auto(const sysobj_virt *vo, const gchar *req) {
    GSList *ret = NULL;
    if (vo && req) {
        gchar *fn = NULL;
        gchar *spath = g_strdup_printf("%s/", req);
        gsize spl = strlen(spath);
        GSList *l = vo_list;
        while (l) {
            sysobj_virt *vo = l->data;
            /* find all vo paths that are immediate children */
            if ( g_str_has_prefix(vo->path, spath) ) {
                if (!strchr(vo->path + spl, '/')) {
                    fn = g_path_get_basename(vo->path);
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
        if (*data == '*') {
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
    if (s && !(s->type & VSO_TYPE_CONST))
        g_free(s);
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

    class_init();
}

void sysobj_cleanup() {
    class_cleanup();
    sysobj_virt_cleanup();
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

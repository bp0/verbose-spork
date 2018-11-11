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

#include "util_sysobj.h"

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

gchar *util_find_line_value(gchar *data, gchar *key, gchar delim) {
    gchar *ret = NULL;
    gchar **lines = g_strsplit(data, "\n", -1);
    gsize line_count = g_strv_length(lines);
    gsize i = 0;
    for (i = 0; i < line_count; i++) {
        gchar *line = lines[i];
        gchar *value = g_utf8_strchr(line, -1, delim);
        if (!value) continue;
        *value = 0;
        value = g_strstrip(value+1);
        gchar *lkey = g_strstrip(line);

        if (!g_strcmp0(lkey, key) ) {
            ret = g_strdup(value);
        }
    }
    g_strfreev(lines);
    return ret;
}

gchar *appf(gchar *src, gchar *fmt, ...) {
    gchar *buf, *ret;
    va_list args;
    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);
    if (src != NULL) {
        ret = g_strdup_printf("%s%s%s", src, strlen(src) ? " " : "", buf);
        g_free(buf);
        g_free(src);
    } else
        ret = buf;
    return ret;
}

gchar *appfs(gchar *src, const gchar *sep, gchar *fmt, ...) {
    gchar *buf, *ret;
    va_list args;
    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);
    if (src != NULL) {
        ret = g_strdup_printf("%s%s%s", src, strlen(src) ? sep : "", buf);
        g_free(buf);
        g_free(src);
    } else
        ret = buf;
    return ret;
}

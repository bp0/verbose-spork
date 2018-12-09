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

static gchar *os_format(sysobj *obj, int fmt_opts);
static const gchar *os_release_label(sysobj *obj);
static gchar *os_release_format(sysobj *obj, int fmt_opts);
static const gchar *lsb_release_label(sysobj *obj);
static gchar *lsb_release_format(sysobj *obj, int fmt_opts);

#define BULLET "\u2022"
#define REFLINK(URI) "<a href=\"" URI "\">" URI "</a>"
const gchar os_release_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.freedesktop.org/software/systemd/man/os-release.html")
    "\n";

const gchar lsb_release_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://wiki.linuxfoundation.org/lsb/start")
    "\n";

static char gen_raw_label[] = N_("Raw operating system release information");

static sysobj_class cls_os_release[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "os_release:raw", .pattern = "/usr/lib/os-release", .flags = OF_CONST,
    .s_halp = os_release_reference_markup_text,
    .s_label = gen_raw_label, .s_suggest = ":/os/os_release" },
  { SYSOBJ_CLASS_DEF
    .tag = "os_release:raw:etc", .pattern = "/etc/os-release", .flags = OF_CONST,
    .s_halp = os_release_reference_markup_text,
    .s_label = gen_raw_label, .s_suggest = ":/os/os_release" },
  { SYSOBJ_CLASS_DEF
    .tag = "lsb_release:raw", .pattern = "/etc/lsb-release", .flags = OF_CONST,
    .s_halp = lsb_release_reference_markup_text,
    .s_label = gen_raw_label, .s_suggest = ":/os/lsb_release" },

  { SYSOBJ_CLASS_DEF
    .tag = "os", .pattern = ":/os", .flags = OF_CONST,
    .f_format = os_format, .s_update_interval = 62.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "os_release", .pattern = ":/os/os_release", .flags = OF_CONST,
    .s_halp = os_release_reference_markup_text, .f_label = os_release_label,
    .f_format = os_release_format, .s_update_interval = 60.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "os_release:item", .pattern = ":/os/os_release/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = os_release_reference_markup_text, .f_label = os_release_label,
    .f_format = os_release_format, .s_update_interval = 30.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "lsb_release", .pattern = ":/os/lsb_release", .flags = OF_CONST,
    .s_halp = lsb_release_reference_markup_text, .f_label = lsb_release_label,
    .f_format = lsb_release_format, .s_update_interval = 60.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "lsb_release:item", .pattern = ":/os/lsb_release/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = lsb_release_reference_markup_text, .f_label = lsb_release_label,
    .f_format = lsb_release_format, .s_update_interval = 30.0 },

};

static gchar *os_format(sysobj *obj, int fmt_opts) {
    gchar *ret = sysobj_format_from_fn(obj->path, "os_release", fmt_opts | FMT_OPT_OR_NULL);
    if (ret)
        return ret;
    ret = sysobj_format_from_fn(obj->path, "lsb_release", fmt_opts | FMT_OPT_OR_NULL);
    if (ret)
        return ret;
    gchar *src = sysobj_raw_from_fn(obj->path, "debian_version");
    if (src) {
        ret = g_strdup_printf("Debian %s", src);
        g_free(src);
        return ret;
    }
    return g_strdup("Linux");
}

static const struct { gchar *rp; gchar *lbl; int extra_flags; } os_release_items[] = {
    { "os_release",  N_("Operating system identification (/usr/lib/os-release)"), OF_NONE },
    { NULL, NULL, 0 }
};

static const struct { gchar *rp; gchar *lbl; int extra_flags; } lsb_release_items[] = {
    { "lsb_release",  N_("Operating system identification (/etc/lsb-release)"), OF_NONE },
    { NULL, NULL, 0 }
};

int os_release_lookup(const gchar *key) {
    int i = 0;
    while(os_release_items[i].rp) {
        if (strcmp(key, os_release_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

const gchar *os_release_label(sysobj *obj) {
    int i = os_release_lookup(obj->name);
    if (i != -1)
        return _(os_release_items[i].lbl);
    return NULL;
}

static const gchar *color_lookup(gchar *ansi_color) {
    static struct { char *ansi, *html; } tab[] = {
        { "0;30", "#010101" },
        { "0;31", "#de382b" },
        { "0;32", "#39b54a" },
        { "0;33", "#ffc706" },
        { "0;34", "#006fb8" },
        { "0;35", "#762671" },
        { "0;36", "#2cb5e9" },
        { "0;37", "#cccccc" },
        { "1;30", "#808080" },
        { "1;31", "#ff0000" },
        { "1;32", "#00ff00" },
        { "1;33", "#ffff00" },
        { "1;34", "#0000ff" },
        { "1;35", "#ff00ff" },
        { "1;36", "#00ffff" },
        { "1;37", "#ffffff" },
    };
    for (int i = 0; i<(int)G_N_ELEMENTS(tab); i++)
        if (!g_strcmp0(tab[i].ansi, ansi_color) )
            return tab[i].html;
    return NULL;
}

static gchar *safe_ansi_color(gchar *ansi_color, gboolean free_in) {
    if (!ansi_color) return NULL;
    int c1 = 0, c2 = 0;
    int mc = sscanf(ansi_color, "%d;%d", &c1, &c2);
    if (free_in)
        g_free(ansi_color);
    if (mc > 1) {
        if (mc == 1) {
            c2 = c1;
            c1 = 0;
        }
        if (c1 == 0 || c1 == 1) {
            if (c2 >= 90 && c2 <= 97) {
                c1 = 1;
                c2 -= 60;
            }
            if (c2 >= 30 && c2 <= 37)
                return g_strdup_printf("%d;%d", c1, c2);
        }
    }
    return NULL;
}

static gchar *format_with_ansi_color(const gchar *str, const gchar *ansi_color, int fmt_opts) {
    gchar *ret = NULL;
    gchar *safe_color = g_strdup(ansi_color);
    util_strstrip_double_quotes_dumb(safe_color);
    safe_color = safe_ansi_color(safe_color, TRUE);
    if (!safe_color)
        goto format_with_ansi_color_end;

    const gchar *html_color = color_lookup(safe_color);

    if (fmt_opts & FMT_OPT_ATERM)
        ret = g_strdup_printf("\x1b[%sm%s" ANSI_COLOR_RESET, safe_color, str);
    else if (fmt_opts & FMT_OPT_PANGO)
        ret = g_strdup_printf("<span background=\"black\" color=\"%s\"> %s </span>", html_color, str);
    else if (fmt_opts & FMT_OPT_HTML)
        ret = g_strdup_printf("<span style=\"background-color: black; color: %s;\"> %s </span>", html_color, str);

format_with_ansi_color_end:
    g_free(safe_color);
    if (!ret)
        ret = g_strdup(str);
    return ret;
}

static gchar *os_release_format(sysobj *obj, int fmt_opts) {
    if (!strcmp("os_release", obj->name)) {
        gchar *ret = NULL;
        gchar *full_name = sysobj_raw_from_fn(":/os/os_release", "PRETTY_NAME");
        gchar *ansi_color = sysobj_raw_from_fn(":/os/os_release", "ANSI_COLOR");
        if (!full_name) {
            gchar *name = sysobj_raw_from_fn(":/os/os_release", "NAME");
            gchar *version = sysobj_raw_from_fn(":/os/os_release", "VERSION");
            util_strstrip_double_quotes_dumb(name);
            util_strstrip_double_quotes_dumb(version);
            if (name)
                full_name = g_strdup_printf("%s %s", name, version ? version : "");
            g_free(name);
            g_free(version);
        } else
            util_strstrip_double_quotes_dumb(full_name);
        if (full_name) {
            ret = full_name;
            if (ansi_color) {
                ret = format_with_ansi_color(full_name, ansi_color, fmt_opts);
                free(full_name);
            }
        } else
            ret = g_strdup("Linux");
        g_free(ansi_color);
        return ret;
    }
    if (!strcmp("ANSI_COLOR", obj->name)) {
        gchar *ret = format_with_ansi_color(obj->data.str, obj->data.str, fmt_opts);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

int lsb_release_lookup(const gchar *key) {
    int i = 0;
    while(lsb_release_items[i].rp) {
        if (strcmp(key, lsb_release_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

const gchar *lsb_release_label(sysobj *obj) {
    int i = lsb_release_lookup(obj->name);
    if (i != -1)
        return _(lsb_release_items[i].lbl);
    return NULL;
}

static gchar *lsb_release_format(sysobj *obj, int fmt_opts) {
    if (!strcmp("lsb_release", obj->name)) {
        gchar *ret = NULL;
        gchar *full_name = sysobj_raw_from_fn(":/os/lsb_release", "DISTRIB_DESCRIPTION");
        if (!full_name) {
            gchar *name = sysobj_raw_from_fn(":/os/lsb_release", "DISTRIB_ID");
            gchar *version = sysobj_raw_from_fn(":/os/lsb_release", "DISTRIB_RELEASE");
            util_strstrip_double_quotes_dumb(name);
            util_strstrip_double_quotes_dumb(version);
            if (name)
                full_name = g_strdup_printf("%s %s", name, version ? version : "");
            g_free(name);
            g_free(version);
        } else
            util_strstrip_double_quotes_dumb(full_name);
        if (full_name) {
            ret = full_name;
        } else
            ret = g_strdup("Linux");
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

void class_os_release() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_os_release); i++) {
        class_add(&cls_os_release[i]);
    }
}

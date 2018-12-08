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
    .tag = "os_release:raw:any_etc", .pattern = "/etc/*-release", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = gen_raw_label, .s_suggest = ":/os" },
  { SYSOBJ_CLASS_DEF
    .tag = "os_release:raw", .pattern = "/etc/lsb-release", .flags = OF_CONST,
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
    gchar *ret = sysobj_format_from_fn(obj->path, "os_release", fmt_opts);
    if (ret)
        return ret;
    ret = sysobj_format_from_fn(obj->path, "lsb_release", fmt_opts);
    if (ret)
        return ret;

    return simple_format(obj, fmt_opts);
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

static gchar *os_release_format(sysobj *obj, int fmt_opts) {
    if (!strcmp("os_release", obj->name)) {
        gchar *ret = NULL;
        gchar *full_name = sysobj_raw_from_fn(":/os/os_release", "PRETTY_NAME");
        gchar *ansi_color = sysobj_raw_from_fn(":/os/os_release", "ANSI_COLOR");
        if (ansi_color)
            util_strstrip_double_quotes_dumb(ansi_color);
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
            if (ansi_color && (fmt_opts & FMT_OPT_ATERM) ) {
                ret = g_strdup_printf("\x1b[%sm%s" ANSI_COLOR_RESET, ansi_color, full_name);
                free(full_name);
            }
        } else
            ret = g_strdup(_("(Unknown)")); /* TODO: check fmt_opts */
        g_free(ansi_color);
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
            ret = g_strdup(_("(Unknown)")); /* TODO: check fmt_opts */
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

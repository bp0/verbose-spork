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
#include "format_funcs.h"

static gchar *os_format(sysobj *obj, int fmt_opts);
static const gchar *os_release_label(sysobj *obj);
static gchar *os_release_format(sysobj *obj, int fmt_opts);
static const gchar *lsb_release_label(sysobj *obj);
static gchar *lsb_release_format(sysobj *obj, int fmt_opts);

const gchar os_release_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.freedesktop.org/software/systemd/man/os-release.html")
    "\n";

const gchar lsb_release_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://wiki.linuxfoundation.org/lsb/start")
    "\n";

static char gen_raw_label[] = N_("Raw operating system release information");

/* incomplete need .f_verify/verify_true() */
attr_tab os_items[] = {
    { "os_release",  N_("operating system identification (/usr/lib/os-release)") },
    { "lsb_release",  N_("operating system identification (/etc/lsb-release)") },
    ATTR_TAB_LAST
};

/* incomplete need .f_verify/verify_true() */
attr_tab os_release_items[] = {
    ATTR_TAB_LAST
};

/* incomplete need .f_verify/verify_true() */
attr_tab lsb_release_items[] = {
    ATTR_TAB_LAST
};

static vendor_list os_vendors(sysobj *obj) {
    gchar *os = auto_free( sysobj_format(obj, FMT_OPT_NONE) );
    return vendor_list_append(NULL, vendor_match(os, NULL) );
}

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
    .tag = "os", .pattern = ":/os", .flags = OF_CONST | OF_HAS_VENDOR,
    .f_vendors = os_vendors, .f_format = os_format, .s_update_interval = 62.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "os:item", .pattern = ":/os/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = os_items, .f_verify = verify_true,
    .f_format = os_format, .s_update_interval = 62.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "os_release", .pattern = ":/os/os_release", .flags = OF_CONST,
    .s_halp = os_release_reference_markup_text,
    .attributes = os_items, .f_format = os_format, .s_update_interval = 39.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "os_release:item", .pattern = ":/os/os_release/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = os_release_items, .f_verify = verify_true,
    .s_halp = os_release_reference_markup_text,
    .f_format = os_release_format, .s_update_interval = 30.0 },

  { SYSOBJ_CLASS_DEF
    .tag = "lsb_release", .pattern = ":/os/lsb_release", .flags = OF_CONST,
    .s_halp = lsb_release_reference_markup_text,
    .attributes = os_items, .f_format = os_format, .s_update_interval = 39.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "lsb_release:item", .pattern = ":/os/lsb_release/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = lsb_release_items, .f_verify = verify_true,
    .s_halp = lsb_release_reference_markup_text,
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

static gchar *os_release_format(sysobj *obj, int fmt_opts) {
    if (SEQ("os_release", obj->name)) {
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

        if (!ansi_color) {
            const Vendor *v = vendor_match(ret, NULL);
            if (v && v->ansi_color && *v->ansi_color) {
                gchar *old = ret;
                ret = format_with_ansi_color(old, v->ansi_color, fmt_opts);
                g_free(old);
            }
        }

        g_free(ansi_color);
        return ret;
    }
    if (SEQ("ANSI_COLOR", obj->name)) {
        gchar *ret = format_with_ansi_color(obj->data.str, obj->data.str, fmt_opts);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static gchar *lsb_release_format(sysobj *obj, int fmt_opts) {
    if (SEQ("lsb_release", obj->name)) {
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
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_os_release); i++)
        class_add(&cls_os_release[i]);
}

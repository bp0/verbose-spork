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

static gchar *os_release_format(sysobj *obj, int fmt_opts);
static double os_release_update_interval(sysobj *obj);

static sysobj_class cls_os_release[] = {
  /* all else */
  { .tag = "os_release", .pattern = ":/os_release", .flags = OF_CONST,
    .f_format = os_release_format, .f_update_interval = os_release_update_interval },
  { .tag = "os_release", .pattern = ":/os_release/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = os_release_format, .f_update_interval = os_release_update_interval },
};

static gchar *os_release_format(sysobj *obj, int fmt_opts) {
    if (!strcmp("os_release", obj->name)) {
        gchar *name = sysobj_raw_from_fn(":/os_release", "NAME");
        gchar *version = sysobj_raw_from_fn(":/os_release", "VERSION");
        util_strstrip_double_quotes_dumb(name);
        util_strstrip_double_quotes_dumb(version);
        gchar *full_name = g_strdup_printf("%s %s", name, version);
        g_free(name);
        g_free(version);
        return full_name;
    }
    return simple_format(obj, fmt_opts);
}

static double os_release_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 60.0; /* there could be an upgrade */
}

void class_os_release() {
    int i = 0;
    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_os_release); i++) {
        class_add(&cls_os_release[i]);
    }
}

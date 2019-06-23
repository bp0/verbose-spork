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

static gboolean backlight_dev_verify(sysobj *obj);
static gchar *backlight_format(sysobj *obj, int fmt_opts);
static gchar *fmt_backlight_type(sysobj *obj, int fmt_opts);
static gchar *fmt_backlight_power(sysobj *obj, int fmt_opts);
static gchar *fmt_brightness(sysobj *obj, int fmt_opts);

const gchar backlight_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-class-backlight")
    "\n";

static attr_tab backlight_items[] = {
    { "bl_power", N_("is on or off"), OF_NONE, fmt_backlight_power, 4.0 },
    { "brightness", N_("requested brightness"), OF_NONE, fmt_brightness, 0.5 },
    { "actual_brightness",  N_("actual brightness"), OF_NONE, fmt_brightness, 0.5 },
    { "max_brightness", N_("maximum brightness"), OF_NONE, fmt_brightness },
    { "type", N_("type of interface"), OF_NONE, fmt_backlight_type },
    ATTR_TAB_LAST
};

static sysobj_class cls_backlight[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "backlight:list", .pattern = "/sys/devices/*/backlight", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = backlight_format, .s_halp = backlight_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "backlight:dev", .pattern = "/sys/devices/*/backlight/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_parent_path_suffix = "/backlight", .s_node_format = "{{bl_power}}{{actual_brightness}}",
    .s_halp = backlight_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "backlight:attr", .pattern = "/sys/devices/*/backlight/*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = backlight_items, .s_halp = backlight_reference_markup_text },

  { SYSOBJ_CLASS_DEF
    .tag = "backlight:dev:intel", .pattern = "/sys/devices/*/intel_backlight", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_node_format = "{{bl_power}}{{actual_brightness}}",
    .s_halp = backlight_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "backlight:attr:intel", .pattern = "/sys/devices/*/intel_backlight/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = backlight_items, .s_halp = backlight_reference_markup_text },
};

static gchar *backlight_format(sysobj *obj, int fmt_opts) {
    const gchar *tag = obj->cls->tag;
    if (SEQ(tag, "backlight:list") ) {
        gchar *ret = NULL;
        for(GSList *l = obj->data.childs; l; l = l->next) {
            sysobj *bl = sysobj_new_from_fn(obj->path, l->data);
            if (bl->cls && SEQ(bl->cls->tag, "backlight:dev") ) {
                gchar *blf = sysobj_format(bl, fmt_opts | FMT_OPT_PART);
                ret = appfs(ret, ", ", "%s", blf );
                g_free(blf);
            }
            sysobj_free(bl);
        }
        if (ret) return ret;
    }
    return simple_format(obj, fmt_opts);
}

static gchar *fmt_brightness(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double lev = strtol(obj->data.str, NULL, 10);
    double max = sysobj_uint32_from_fn(obj->path, "../max_brightness", 10);
    if (max == 0.0) max = 255;
    lev = (lev / max) * 100;
    g_free(trim);
    return g_strdup_printf("%.1lf%s", lev, _("%") );
}

static gchar *fmt_backlight_type(sysobj *obj, int fmt_opts) {
    static lookup_tab backlight_type[] = {
        { "firmware", N_("the driver uses a standard firmware interface") },
        { "platform", N_("the driver uses a platform-specific interface") },
        { "raw", N_("the driver controls hardware registers directly") },
        LOOKUP_TAB_LAST
    };
    gchar *ret = sysobj_format_lookup_tab(obj, backlight_type, fmt_opts);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static gchar *fmt_backlight_power(sysobj *obj, int fmt_opts) {
    static lookup_tab backlight_bl_power[] = {
        { "0", N_("power on"), N_("on") },
        { "4", N_("power off"), N_("off") },
        LOOKUP_TAB_LAST
    };
    gchar *ret = sysobj_format_lookup_tab(obj, backlight_bl_power, fmt_opts);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

void class_backlight() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_backlight); i++)
        class_add(&cls_backlight[i]);
}

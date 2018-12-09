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

const gchar devfreq_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-devfreq")
    "\n";

static gboolean devfreq_verify(sysobj *obj);
static gboolean devfreq_dev_verify(sysobj *obj);
static gboolean devfreq_item_verify(sysobj *obj);
static const gchar *devfreq_label(sysobj *obj);
static gchar *devfreq_format(sysobj *obj, int fmt_opts);
static double devfreq_update_interval(sysobj *obj);

static sysobj_class cls_devfreq[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "devfreq", .pattern = "/sys/devices/*/devfreq", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = devfreq_reference_markup_text,
    .f_verify = devfreq_verify, .f_format = devfreq_format },
  { SYSOBJ_CLASS_DEF
    .tag = "devfreq:dev", .pattern = "/sys/devices/*/devfreq/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = devfreq_reference_markup_text, .s_update_interval = 0.5,
    .f_verify = devfreq_dev_verify, .f_format = devfreq_format },
  { SYSOBJ_CLASS_DEF
    .tag = "devfreq:dev:item", .pattern = "/sys/devices/*/devfreq/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = devfreq_reference_markup_text, .f_label = devfreq_label,
    .f_verify = devfreq_item_verify, .f_format = devfreq_format, .f_update_interval = devfreq_update_interval },
};

static const struct { gchar *item; gchar *lbl; int extra_flags; } devfreq_items[] = {
    { "governor",  N_("devfreq governor"), OF_NONE },
    { "min_freq",  N_("minimum frequency requested"), OF_NONE },
    { "max_freq",  N_("maximum frequency requested"), OF_NONE },
    { "cur_freq",  N_("current frequency"), OF_NONE },
    { "target_freq",  N_("next governor-predicted target frequency"), OF_NONE },
    { "polling_interval",  N_("requested polling interval"), OF_NONE },
    { "trans_stat",  N_("statistics of devfreq behavior"), OF_NONE },
    { "available_frequencies",  N_("available frequencies"), OF_NONE },
    { "available_governors",  N_("available devfreq governors"), OF_NONE },
    { NULL, NULL, 0 }
};

int devfreq_lookup(const gchar *key) {
    int i = 0;
    while(devfreq_items[i].item) {
        if (SEQ(key, devfreq_items[i].item))
            return i;
        i++;
    }
    return -1;
}

const gchar *devfreq_label(sysobj *obj) {
    int i = devfreq_lookup(obj->name);
    if (i != -1)
        return _(devfreq_items[i].lbl);
    return NULL;
}

static gboolean devfreq_verify(sysobj *obj) {
    if (SEQ("devfreq", obj->name) )
        return TRUE;
    return FALSE;
}

static gboolean devfreq_dev_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pn = sysobj_parent_name(obj);
    if (SEQ("devfreq", pn) )
        ret = TRUE;
    g_free(pn);
    return ret;
}

static gboolean devfreq_item_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *chkpath = util_build_fn(obj->path, "../..");
    sysobj *ppo = sysobj_new_fast(chkpath);
    if (SEQ("devfreq", ppo->name) )
        ret = TRUE;
    sysobj_free(ppo);
    g_free(chkpath);
    if (!ret) return FALSE;

    int i = devfreq_lookup(obj->name);
    if (i != -1)
        return TRUE;
    return FALSE;
}

static gchar *devfreq_format(sysobj *obj, int fmt_opts) {
    if ( SEQ(obj->cls->tag, "devfreq:dev") ) {
        gchar *ret = NULL;
        /* get nice description */
        gchar *gov =
            sysobj_format_from_fn(obj->path, "governor", fmt_opts | FMT_OPT_PART);
        gchar *cur =
            sysobj_format_from_fn(obj->path, "cur_freq", fmt_opts | FMT_OPT_PART);

        if (fmt_opts & FMT_OPT_SHORT)
            ret = g_strdup_printf("%s", cur);
        else
            ret = g_strdup_printf("%s [ %s ]",
                    gov, cur );
        g_free(gov);
        g_free(cur);
        return ret;
    }

    if ( SEQ(obj->cls->tag, "devfreq:dev:item") ) {
        if ( g_str_has_suffix(obj->name, "_freq") )
            return fmt_hz_to_mhz(obj, fmt_opts);
        if ( SEQ(obj->name, "polling_interval") )
            return fmt_milliseconds(obj, fmt_opts);
    }

    return simple_format(obj, fmt_opts);
}

static double devfreq_update_interval(sysobj *obj) {
    if ( SEQ(obj->cls->tag, "devfreq:dev:item") ) {
        if ( g_str_has_suffix(obj->name, "_freq") )
            return 0.1;
    }
    return 1.7;
}

void class_devfreq() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_devfreq); i++)
        class_add(&cls_devfreq[i]);
}

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
#include "sysobj_extras.h" /* for dtr_compat_decode() */

static const gchar *gpu_label(sysobj *s);
static gchar *gpu_format(sysobj *obj, int fmt_opts);

static gboolean drm_card_verify(sysobj *obj);
static gchar *drm_card_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_gpu[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "gpu", .pattern = ":/gpu*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_label = gpu_label, .f_format = gpu_format },

  { SYSOBJ_CLASS_DEF
    .tag = "drm:card", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("DRM-managed graphics device"),
    .f_verify = drm_card_verify, .f_format = drm_card_format },
};

static const struct { gchar *item; gchar *lbl; int extra_flags; } gpu_items[] = {
    //{ "raspberry_pi",  N_("Raspberry Pi Information"), OF_NONE },
    { NULL, NULL, 0 }
};

int gpu_lookup(const gchar *key) {
    int i = 0;
    while(gpu_items[i].item) {
        if (SEQ(key, gpu_items[i].item))
            return i;
        i++;
    }
    return -1;
}

const gchar *gpu_label(sysobj *obj) {
    int i = gpu_lookup(obj->name);
    if (i != -1)
        return _(gpu_items[i].lbl);
    return NULL;
}

static gchar *gpu_format(sysobj *obj, int fmt_opts) {
    if (SEQ(":/gpu", obj->path)) {
        //summary
    }
    return simple_format(obj, fmt_opts);
}

static gboolean drm_card_verify(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *pn = sysobj_parent_name(obj);
    if (SEQ(pn, "drm")
        && verify_lblnum(obj, "card") )
        ret = TRUE;
    g_free(pn);
    return ret;
}

static gchar *drm_card_format(sysobj *obj, int fmt_opts) {
    gchar *device_name = sysobj_format_from_fn(obj->path, "device", fmt_opts | FMT_OPT_NULL_IF_SIMPLE_DIR);
    if (device_name)
        return device_name;

    sysobj *dt_compat_obj = sysobj_new_from_fn(obj->path, "device/of_node/compatible");
    if (dt_compat_obj->exists) {
        sysobj_read(dt_compat_obj, FALSE);
        gchar *dt_name = dtr_compat_decode(dt_compat_obj->data.any, dt_compat_obj->data.len, FALSE);
        sysobj_free(dt_compat_obj);
        if (dt_name)
            return dt_name;
    } else
        sysobj_free(dt_compat_obj);

    return simple_format(obj, fmt_opts);
}

void class_gpu() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_gpu); i++)
        class_add(&cls_gpu[i]);
}

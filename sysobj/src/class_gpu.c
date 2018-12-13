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
#include "format_funcs.h"

static gboolean gpu_verify(sysobj *obj);
static gchar *gpu_format(sysobj *obj, int fmt_opts);

static gboolean gpu_prop_verify(sysobj *obj);
static const gchar *gpu_prop_label(sysobj *s);
static gchar *gpu_prop_format(sysobj *obj, int fmt_opts);

static gboolean drm_card_verify(sysobj *obj);
static gchar *drm_card_format(sysobj *obj, int fmt_opts);

static gboolean drm_card_attr_verify(sysobj *obj);
static const gchar *drm_card_attr_label(sysobj *s);
static gchar *drm_card_attr_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_gpu[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "gpu", .pattern = ":/gpu*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = gpu_verify, .f_format = gpu_format },
  { SYSOBJ_CLASS_DEF
    .tag = "gpu:prop", .pattern = ":/gpu/gpu*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = gpu_prop_verify, .f_format = gpu_prop_format, .f_label = gpu_prop_label },

  { SYSOBJ_CLASS_DEF
    .tag = "drm:card", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("DRM-managed graphics device"),
    .f_verify = drm_card_verify, .f_format = drm_card_format },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:card:attr", .pattern = "/sys/devices/*/drm/card*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_label = drm_card_attr_label, .f_verify = drm_card_attr_verify, .f_format = drm_card_attr_format },
};

static gboolean gpu_verify(sysobj *obj) {
    if (SEQ(":/gpu", obj->path))
        return TRUE;
    if (verify_lblnum(obj, "gpu"))
        return TRUE;
    return FALSE;
}

static gchar *gpu_format(sysobj *obj, int fmt_opts) {
    if (SEQ(":/gpu", obj->path)) {
        return sysobj_format_from_fn(obj->path, "list", fmt_opts);
    }
    if (verify_lblnum(obj, "gpu"))
        return sysobj_format_from_fn(obj->path, "name", fmt_opts);
    return simple_format(obj, fmt_opts);
}

static const struct { gchar *item; gchar *lbl; int extra_flags; func_format f_func; } gpu_prop_items[] = {
    { "opp.khz_min",   "operating-performance-points minimum frequency", OF_NONE, fmt_khz },
    { "opp.khz_max",   "operating-performance-points maximum frequency", OF_NONE, fmt_khz },
    { "opp.clock_frequency",  "devicetree-specified clock frequency", OF_NONE, fmt_khz },
    { "opp.clock_latency_ns", "transition latency", OF_NONE, fmt_nanoseconds },
    { NULL, NULL, 0 }
};

int gpu_prop_lookup(const gchar *key) {
    int i = 0;
    while(gpu_prop_items[i].item) {
        if (SEQ(key, gpu_prop_items[i].item))
            return i;
        i++;
    }
    return -1;
}

const gchar *gpu_prop_label(sysobj *obj) {
    int i = gpu_prop_lookup(obj->name);
    if (i != -1)
        return _(gpu_prop_items[i].lbl);
    return NULL;
}

static gboolean gpu_prop_verify(sysobj *obj) {
    if (verify_lblnum_child(obj, "gpu")) {
        int i = gpu_prop_lookup(obj->name);
        if (i != -1)
            return TRUE;
    }
    return FALSE;
}

static gchar *gpu_prop_format(sysobj *obj, int fmt_opts) {
    int i = gpu_prop_lookup(obj->name);
    if (i != -1) {
        if (gpu_prop_items[i].f_func)
            return gpu_prop_items[i].f_func(obj, fmt_opts);
    }
    return simple_format(obj, fmt_opts);
}

static const struct { gchar *item; gchar *lbl; int extra_flags; func_format f_func; } drm_items[] = {
    //TODO: labels
    { "gt_cur_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_min_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_max_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_boost_freq_mhz", NULL, OF_NONE, fmt_mhz },
    { "gt_act_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_RP0_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_RP1_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_RPn_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { NULL, NULL, 0 }
};

int drm_attr_lookup(const gchar *key) {
    int i = 0;
    while(drm_items[i].item) {
        if (SEQ(key, drm_items[i].item))
            return i;
        i++;
    }
    return -1;
}

const gchar *drm_card_attr_label(sysobj *obj) {
    int i = drm_attr_lookup(obj->name);
    if (i != -1)
        return _(drm_items[i].lbl);
    return NULL;
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

static gboolean drm_card_attr_verify(sysobj *obj) {
    if (verify_lblnum_child(obj, "card") ) {
        int i = drm_attr_lookup(obj->name);
        if (i != -1)
            return TRUE;
    }
    if (g_str_has_suffix(obj->name, "_freq_mhz") )
        return TRUE;
    return FALSE;
}

static gchar *drm_card_attr_format(sysobj *obj, int fmt_opts) {
    int i = drm_attr_lookup(obj->name);
    if (i != -1) {
        if (drm_items[i].f_func)
            return drm_items[i].f_func(obj, fmt_opts);
    }
    if (g_str_has_suffix(obj->name, "_freq_mhz") )
        return fmt_mhz(obj, fmt_opts);

    return simple_format(obj, fmt_opts);
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

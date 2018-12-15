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

static gboolean drm_card_verify(sysobj *obj);
static gchar *drm_card_format(sysobj *obj, int fmt_opts);

static gboolean drm_card_attr_verify(sysobj *obj);
static gchar *drm_card_attr_format(sysobj *obj, int fmt_opts);

static gboolean drm_conn_verify(sysobj *obj);
static gchar *drm_conn_format(sysobj *obj, int fmt_opts);

static gboolean drm_conn_attr_verify(sysobj *obj);

attr_tab gpu_prop_items[] = {
    { "name", NULL, OF_NONE, NULL, -1 },
    { "opp.khz_min",   "operating-performance-points minimum frequency", OF_NONE, fmt_khz_to_mhz, -1 },
    { "opp.khz_max",   "operating-performance-points maximum frequency", OF_NONE, fmt_khz_to_mhz, -1 },
    { "opp.clock_frequency",  "devicetree-specified clock frequency", OF_NONE, fmt_khz_to_mhz, -1 },
    { "opp.clock_latency_ns", "transition latency", OF_NONE, fmt_nanoseconds, -1 },
    ATTR_TAB_LAST
};

attr_tab drm_items[] = {
    //TODO: labels
    { "gt_cur_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_min_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_max_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_boost_freq_mhz", NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_act_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_RP0_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_RP1_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    { "gt_RPn_freq_mhz",   NULL, OF_NONE, fmt_mhz, -1 },
    ATTR_TAB_LAST
};

attr_tab drm_conn_items[] = {
    { "status" },
    { "enabled" },
    { "modes" },
    { "edid" },
    { "dpms" },
    ATTR_TAB_LAST
};

static sysobj_class cls_gpu[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "gpu", .pattern = ":/gpu*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = gpu_verify, .f_format = gpu_format },
  { SYSOBJ_CLASS_DEF
    .tag = "gpu:prop", .pattern = ":/gpu/gpu*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = gpu_prop_items },

  { SYSOBJ_CLASS_DEF
    .tag = "drm:card", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("DRM-managed graphics device"),
    .f_verify = drm_card_verify, .f_format = drm_card_format },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:card:attr", .pattern = "/sys/devices/*/drm/card*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = drm_card_attr_verify, .f_format = drm_card_attr_format,
    .attributes = drm_items },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:link", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_label = N_("DRM connection"),
    .f_verify = drm_conn_verify, .f_format = drm_conn_format },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:link:attr", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = drm_conn_attr_verify, .attributes = drm_conn_items },
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

static gboolean drm_card_attr_verify(sysobj *obj) {
    if (verify_lblnum_child(obj, "card") ) {
        int i = attr_tab_lookup(drm_items, obj->name);
        if (i != -1)
            return TRUE;
    }
    if (g_str_has_suffix(obj->name, "_freq_mhz") )
        return TRUE;
    return FALSE;
}

static gchar *drm_card_attr_format(sysobj *obj, int fmt_opts) {
    int i = attr_tab_lookup(drm_items, obj->name);
    if (i != -1) {
        if (drm_items[i].fmt_func)
            return drm_items[i].fmt_func(obj, fmt_opts);
    }
    if (g_str_has_suffix(obj->name, "_freq_mhz") )
        return fmt_mhz(obj, fmt_opts);

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

static gboolean drm_conn_verify(sysobj *obj) {
    gboolean ret = FALSE;
    if (verify_lblnum_child(obj, "card") ) {
        sysobj *pobj = sysobj_parent(obj, FALSE);
        if (g_str_has_prefix(obj->name, pobj->name) ) {
            ret = TRUE;
        }
        sysobj_free(pobj);
    }
    return ret;
}

static gchar *drm_conn_format(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    gchar *status = sysobj_format_from_fn(obj->path, "status", fmt_opts);
    gchar *enabled = sysobj_format_from_fn(obj->path, "enabled", fmt_opts);
    if (status && enabled)
        ret = g_strdup_printf("%s; %s", enabled, status);
    g_free(status);
    g_free(enabled);
    if (ret)
        return ret;
    return simple_format(obj, fmt_opts);
}

static gboolean drm_conn_attr_verify(sysobj *obj) {
    gboolean ret = FALSE;
    sysobj *pobj = sysobj_parent(obj, FALSE);
    if (pobj->cls)
        if (SEQ(pobj->cls->tag, "drm:link") )
            ret = verify_in_attr_tab(obj, drm_conn_items);
    sysobj_free(pobj);
    return ret;
}

void class_gpu() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_gpu); i++)
        class_add(&cls_gpu[i]);
}

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

static gboolean gpu_list_verify(sysobj *obj);
static gchar *gpu_list_format(sysobj *obj, int fmt_opts);

static gboolean gpu_verify(sysobj *obj);
static gchar* gpu_format_nice_name(sysobj *gpu, int fmt_opts);
static gboolean gpu_prop_verify(sysobj *obj);

static vendor_list gpu_vendors(sysobj *obj);
static vendor_list gpu_all_vendors(sysobj *obj);

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
    .tag = "gpu:list", .pattern = ":/gpu", .flags = OF_GLOB_PATTERN | OF_CONST | OF_IS_VENDOR,
    .s_label = N_("list of discovered graphics processor devices"),
    .f_verify = gpu_list_verify, .f_format = gpu_list_format, .f_vendors = gpu_all_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "gpu", .pattern = ":/gpu/gpu*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_IS_VENDOR,
    .s_label = N_("graphics device"),
    .f_verify = gpu_verify, .f_format = gpu_format_nice_name, .f_vendors = gpu_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "gpu:prop", .pattern = ":/gpu/gpu*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = gpu_prop_verify, .attributes = gpu_prop_items },

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

vendor_list gpu_vendors(sysobj *obj) {
    gchar *vs = sysobj_raw_from_fn(obj->path, "name/vendor_name");
    const Vendor *v = vendor_match(vs, NULL);
    g_free(vs);
    if (v)
        return vendor_list_append(NULL, v);
    return NULL;
}

static vendor_list gpu_all_vendors(sysobj *obj) {
    GSList *ret = NULL;
    if (SEQ(":/gpu", obj->path)) {
        GSList *childs = sysobj_children(obj, "gpu*", NULL, TRUE);
        for(GSList *l = childs; l; l = l->next)
            ret = vendor_list_concat(ret, sysobj_vendors_from_fn(obj->path, l->data));
        g_slist_free_full(childs, g_free);
    }
    return ret;
}

static gboolean gpu_list_verify(sysobj *obj) {
    if (SEQ(":/gpu", obj->path))
        return TRUE;
    return FALSE;
}

static gchar *gpu_list_format(sysobj *obj, int fmt_opts) {
    if (SEQ(":/gpu", obj->path)) {
        gchar *ret = NULL;
        GSList *childs = sysobj_children(obj, "gpu*", NULL, TRUE);
        for(GSList *l = childs; l; l = l->next) {
            gchar *gpu = sysobj_format_from_fn(obj->path, l->data, fmt_opts);
            if (gpu)
                ret = appfs(ret, " + ", "%s", gpu);
            g_free(gpu);
        }
        g_slist_free_full(childs, g_free);
        if (ret)
            return ret;
    }
    return simple_format(obj, fmt_opts);
}

static gboolean gpu_verify(sysobj *obj) {
    if (verify_lblnum(obj, "gpu"))
        return TRUE;
    return FALSE;
}

static gboolean gpu_prop_verify(sysobj *obj) {
    return (verify_lblnum_child(obj, "gpu") &&
        verify_in_attr_tab(obj, gpu_prop_items) );
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

/* export */
gboolean str_shorten(gchar *str, const gchar *find, const gchar *replace) {
    if (!str || !find || !replace) return FALSE;
    long unsigned lf = strlen(find);
    long unsigned lr = strlen(replace);
    gchar *p = strstr(str, find);
    if (p) {
        if (lr > lf) lr = lf;
        gchar *buff = g_strnfill(lf, ' ');
        strncpy(buff, replace, lr);
        strncpy(p, buff, lf);
        g_free(buff);
        return TRUE;
    }
    return FALSE;
}

/* TODO: In the future, when there is more vendor specific information available in
 * the gpu struct, then more precise names can be given to each gpu */
static gchar* gpu_format_nice_name(sysobj *gpu, int fmt_opts) {
    static const gchar unk_v[] = "Unknown"; /* do not...    */
    static const gchar unk_d[] = "Device";  /* ...translate */

    gchar *nice_name = NULL;

    gchar *vendor_str = sysobj_raw_from_fn(gpu->path, "name/vendor_name");
    gchar *vendor_short = sysobj_raw_from_fn(gpu->path, "name/vendor/name_short");
    gchar *device_str = sysobj_raw_from_fn(gpu->path, "name/device_name");
    gchar *ansi_color = sysobj_raw_from_fn(gpu->path, "name/vendor/ansi_color");
    gchar *dt_compat = sysobj_raw_from_fn(gpu->path, "/device/of_node/compatible");

    if (ansi_color)
        ansi_color = safe_ansi_color_ex(ansi_color, TRUE);
    if (vendor_short) {
        g_free(vendor_str);
        vendor_str = vendor_short;
        vendor_short = NULL;
    }
    if (vendor_str && ansi_color) {
        gchar *color_ven = format_with_ansi_color(vendor_str, ansi_color, fmt_opts);
        g_free(vendor_str);
        vendor_str = color_ven;
    }
    gchar *nv_model = sysobj_raw_from_fn(gpu->path, "/nvidia/model");
    /* NV information available */
    if (nv_model) {
        nice_name = g_strdup_printf("%s %s", vendor_str, nv_model);
        goto nice_is_over;
    }

    if (dt_compat) {
        if (!vendor_str && !device_str) {
            nice_name = g_strdup(dt_compat);
            char *comma = strchr(nice_name, ',');
            if (comma) *comma = ' ';  /* "brcm,bcm2835-vc4" -> "brcm bcm2835-vc4" */
            goto nice_is_over;
        }
        if (!device_str) {
            char *comma = strchr(dt_compat, ',');
            if (comma) {
                nice_name = g_strdup_printf("%s %s", vendor_str, comma + 1);
                goto nice_is_over;
            }
        }
        if (!vendor_str) {
            nice_name = g_strdup_printf("%s", device_str);
            goto nice_is_over;
        }
    }

    if (!vendor_str) vendor_str = g_strdup(unk_v);
    if (!device_str) device_str = g_strdup(unk_d);

    /* These two former special cases are currently handled by the vendor_get_shortest_name()
     * function well enough, but the notes are preserved here. */
        /* nvidia PCI strings are pretty nice already,
         * just shorten the company name */
        // s->nice_name = g_strdup_printf("%s %s", "nVidia", device_str);
        /* Intel Graphics may have very long names, like "Intel Corporation Seventh Generation Something Core Something Something Integrated Graphics Processor Revision Ninety-four"
         * but for now at least shorten "Intel Corporation" to just "Intel" */
        // s->nice_name = g_strdup_printf("%s %s", "Intel", device_str);
    if (strstr(vendor_str, "Intel")) {
        gchar *full_name = strdup(device_str);
        str_shorten(full_name, "(R)", ""); /* Intel(R) -> Intel */
        str_shorten(full_name, "Integrated Graphics Controller", "Integrated Graphics");
        str_shorten(full_name, "Generation", "Gen");
        str_shorten(full_name, "Core Processor", "Core");
        str_shorten(full_name, "Atom Processor", "Atom");
        util_compress_space(full_name);
        g_strstrip(full_name);
        nice_name = g_strdup_printf("%s %s", vendor_str, full_name);
        g_free(full_name);
        goto nice_is_over;
    }

    if (strstr(vendor_str, "AMD")) {
        /* AMD PCI strings are crazy stupid because they use the exact same
         * chip and device id for a zillion "different products" */
        gchar *full_name = strdup(device_str);
        /* Try and shorten it to the chip code name only, at least */
        char *b = strchr(full_name, '[');
        if (b) *b = 0;
        g_strstrip(full_name);
        nice_name = g_strdup_printf("%s %s", vendor_str, full_name);
        g_free(full_name);
        goto nice_is_over;
    }

    /* nothing nicer */
    nice_name = g_strdup_printf("%s %s", vendor_str, device_str);

nice_is_over:
    g_free(nv_model);
    g_free(device_str);
    g_free(vendor_str);
    g_free(dt_compat);
    g_free(ansi_color);
    return nice_name;
}

void class_gpu() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_gpu); i++)
        class_add(&cls_gpu[i]);
}

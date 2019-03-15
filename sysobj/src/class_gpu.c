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

static gboolean drm_conn_attr_verify(sysobj *obj);

static attr_tab gpu_prop_items[] = {
    { "name", NULL, OF_NONE, NULL, UPDATE_INTERVAL_NEVER },
    { "driver_kmod", N_("driver kernel module") },
    { "core_khz_cur", N_("current clock frequency"), OF_NONE, fmt_khz_to_mhz, 0.5 },
    { "core_khz_min", N_("minimum clock frequency"), OF_NONE, fmt_khz_to_mhz, 5.0 },
    { "core_khz_max", N_("maximum clock frequency"), OF_NONE, fmt_khz_to_mhz, 5.0 },
    { "opp.khz_min", N_("operating-performance-points minimum frequency"), OF_NONE, fmt_khz_to_mhz, UPDATE_INTERVAL_NEVER },
    { "opp.khz_max", N_("operating-performance-points maximum frequency"), OF_NONE, fmt_khz_to_mhz, UPDATE_INTERVAL_NEVER },
    { "opp.clock_frequency", N_("devicetree-specified clock frequency"), OF_NONE, fmt_khz_to_mhz, UPDATE_INTERVAL_NEVER },
    { "opp.clock_latency_ns", N_("transition latency"), OF_NONE, fmt_nanoseconds, UPDATE_INTERVAL_NEVER },
    ATTR_TAB_LAST
};

static attr_tab drm_items[] = {
    //TODO: labels
    { "gt_cur_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_min_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_max_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_boost_freq_mhz", NULL, OF_NONE, fmt_mhz },
    { "gt_act_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_RP0_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_RP1_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    { "gt_RPn_freq_mhz",   NULL, OF_NONE, fmt_mhz },
    ATTR_TAB_LAST
};

/* just enough edid decoding */
struct edid {
    int ver_major;
    int ver_minor;

    gchar ven[4];

    int d_type[4];
    gchar d_text[4][14];

    /* point into d_text */
    gchar *name;
    gchar *serial;
    gchar *ut1;
    gchar *ut2;

    int a_or_d; /* 0 = analog, 1 = digital */
    int bpc;

    uint16_t product;
    uint32_t n_serial;
    int week;
    int year;

    int horiz_cm;
    int vert_cm;

    int check;
};

static const gchar *edid_descriptor_type(int type) {
    switch(type) {
        case 0xff:
            return N_("display serial number");
        case 0xfe:
            return N_("unspecified text");
        case 0xfd:
            return N_("display range limits");
        case 0xfc:
            return N_("display name");
        case 0xfb:
            return N_("additional white point");
        case 0xfa:
            return N_("additional standard timing identifiers");
        case 0xf9:
            return N_("Display Color Management");
        case 0xf8:
            return N_("CVT 3-byte timing codes");
        case 0xf7:
            return N_("additional standard timing");
        case 0x10:
            return N_("dummy");
    }
    if (type < 0x0f) return N_("manufacturer reserved descriptor");
    return N_("detailed timing descriptor");
}

static void fill_edid(struct edid *id_out, sysobj *obj) {
    if (!id_out) return;

    memset(id_out, 0, sizeof(struct edid));

    if (obj->data.len >= 128) {
        uint8_t *u8 = obj->data.uint8;
        uint16_t *u16 = obj->data.uint16;
        uint32_t *u32 = obj->data.uint32;

        uint16_t vid = be16toh(u16[4]); /* bytes 8-9 */
        id_out->ven[2] = 64 + (vid & 0x1f);
        id_out->ven[1] = 64 + ((vid >> 5) & 0x1f);
        id_out->ven[0] = 64 + ((vid >> 10) & 0x1f);

        id_out->product = le16toh(u16[5]); /* bytes 10-11 */
        id_out->n_serial = le32toh(u32[3]);/* bytes 12-15 */
        id_out->week = u8[16];             /* byte 16 */
        id_out->year = u8[17] + 1990;      /* byte 17 */
        id_out->ver_major = u8[18];        /* byte 18 */
        id_out->ver_minor = u8[19];        /* byte 19 */

        id_out->a_or_d = (u8[20] & 0x80) ? 1 : 0;
        if (id_out->a_or_d == 1) {
            /* digital */
            switch((u8[20] >> 4) & 0x7) {
                case 0x1: id_out->bpc = 6; break;
                case 0x2: id_out->bpc = 8; break;
                case 0x3: id_out->bpc = 10; break;
                case 0x4: id_out->bpc = 12; break;
                case 0x5: id_out->bpc = 14; break;
                case 0x6: id_out->bpc = 16; break;
            }
        }

        id_out->horiz_cm = u8[21];
        id_out->vert_cm = u8[22];

        id_out->check = u8[127];

        uint16_t dh, dl;

        /* descriptor at byte 54 */
        dh = be16toh(u16[54/2]);
        dl = be16toh(u16[54/2+1]);
        id_out->d_type[0] = (dh << 16) + dl;
        switch(id_out->d_type[0]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[0], u8+54+5, 13);
        }

        /* descriptor at byte 72 */
        dh = be16toh(u16[72/2]);
        dl = be16toh(u16[72/2+1]);
        id_out->d_type[1] = (dh << 16) + dl;
        switch(id_out->d_type[1]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[1], u8+72+5, 13);
        }

        /* descriptor at byte 90 */
        dh = be16toh(u16[90/2]);
        dl = be16toh(u16[90/2+1]);
        id_out->d_type[2] = (dh << 16) + dl;
        switch(id_out->d_type[2]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[2], u8+90+5, 13);
        }

        /* descriptor at byte 108 */
        dh = be16toh(u16[108/2]);
        dl = be16toh(u16[108/2+1]);
        id_out->d_type[3] = (dh << 16) + dl;
        switch(id_out->d_type[3]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[3], u8+108+5, 13);
        }

        for(int i = 0; i < 4; i++) {
            g_strstrip(id_out->d_text[i]);
            switch(id_out->d_type[i]) {
                case 0xfc:
                    id_out->name = id_out->d_text[i];
                    break;
                case 0xff:
                    id_out->serial = id_out->d_text[i];
                    break;
                case 0xfe:
                    if (id_out->ut1)
                        id_out->ut2 = id_out->d_text[i];
                    else
                        id_out->ut1 = id_out->d_text[i];
                    break;
            }
        }

        if (!id_out->name) {
            /* LG may use "uspecified text" for name and model */
            //SEQ(id_out->ven, "LGD")
            if (SEQ(id_out->ut1, "LG Display") && id_out->ut2)
                id_out->name = id_out->ut2;
            else {
                if (id_out->ut1) id_out->name = id_out->ut1;
                if (id_out->ut2 && !id_out->serial) id_out->serial = id_out->ut2;
            }
        }
    }
}

vendor_list edid_vendor(sysobj *obj) {
    struct edid id = {};
    fill_edid(&id, obj);
    const Vendor *v = vendor_match(id.ven, NULL);
    return vendor_list_append(NULL, v);
}

gchar *edid_format(sysobj *obj, int fmt_opts) {
    struct edid id = {};
    fill_edid(&id, obj);
    gchar *ret = NULL;
    if (id.ver_major) {
        if (fmt_opts & FMT_OPT_PART || !(fmt_opts & FMT_OPT_COMPLETE) ) {
            gchar *ven_tag = vendor_match_tag(id.ven, fmt_opts);
            if (ven_tag)
                ret = appf(ret, "%s", ven_tag);
            else
                ret = appf(ret, "%s", id.ven);
            g_free(ven_tag);

            if (id.name)
                ret = appf(ret, "%s", id.name);
            else if (ret) {
                ret = appf(ret, "%s %s", id.a_or_d ? "Digital" : "Analog", "Display");
            }

        } else if (fmt_opts & FMT_OPT_COMPLETE) {
            ret = appfs(ret, "\n", "edid_version: %d.%d", id.ver_major, id.ver_minor);
            ret = appfs(ret, "\n", "mfg: %s, model: %u, n_serial: %u, dom: week %d of %d", id.ven, id.product, id.n_serial, id.week, id.year);

            ret = appfs(ret, "\n", "type: %s", id.a_or_d ? "digital" : "analog");
            if (id.bpc)
                ret = appfs(ret, "\n", "bits per color channel: %d", id.bpc);

            if (id.horiz_cm && id.vert_cm)
                ret = appfs(ret, "\n", "size: %d cm × %d cm", id.horiz_cm, id.vert_cm);
            ret = appfs(ret, "\n", "checkbyte: %d", id.check);

            for(int i = 0; i < 4; i++)
                ret = appfs(ret, "\n", "descriptor[%d] (%s): %s", i, _(edid_descriptor_type(id.d_type[i])), *id.d_text[i] ? id.d_text[i] : "{...}");

        }
    }
    if (ret)
        return ret;
    return simple_format(obj, fmt_opts);
}

static attr_tab drm_conn_items[] = {
    { "status" },
    { "enabled" },
    { "modes" },
    { "edid", N_("Extended Display Identification Data"), OF_HAS_VENDOR, edid_format },
    { "dpms", N_("VESA Display Power Management Signaling") },
    ATTR_TAB_LAST
};

/* <id>: <value>[*] */
/* ... or <id>[*]: <table columns> */
static gchar *fmt_amdgpu_val(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    gboolean want_complete = fmt_opts & FMT_OPT_COMPLETE;
    gchar **list = g_strsplit(obj->data.str, "\n", -1);
    for (int i = 0; list[i]; i++) {
        if (!strlen(list[i])) continue;
        if (strchr(list[i], '*') || want_complete) {
                gchar *c = strchr(list[i], ':');
                gchar *a = strchr(list[i], '*');
                if (c) {
                    *c = 0;
                    if (!want_complete && a > c) *a = 0;
                    c = g_strstrip(c+1);
                    ret = appfs(ret, "\n", "[%s] %s", list[i], c);
                } else
                    ret = appfs(ret, "\n", "%s", list[i]);
        }
    }
    g_strfreev(list);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

/* format: "0: 300Mhz *" */
static gchar *fmt_amdgpu_clk(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    gboolean want_complete = fmt_opts & FMT_OPT_COMPLETE;
    gchar **list = g_strsplit(obj->data.str, "\n", -1);
    for (int i = 0; list[i]; i++) {
        if (!strlen(list[i])) continue;
        if (strchr(list[i], '*') || want_complete) {
                gchar *c = strchr(list[i], ':');
                gchar *a = strchr(list[i], '*');
                if (c) {
                    *c = 0;
                    c = g_strstrip(c+1);
                    double mhz = c ? strtod(c, NULL) : 0.0;
                    ret = (fmt_opts & FMT_OPT_NO_UNIT)
                        ? appfs(ret, "\n", "%0.3lf", mhz)
                        : appfs(ret, "\n", "[%s] %0.3lf %s%s", list[i], mhz, _("MHz"), (a && want_complete) ? " *" : "");
                } else
                    ret = appfs(ret, "\n", "%s", list[i]);
        }
    }
    g_strfreev(list);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

/* format "0 NAME*: TABLE COLUMNS" */
static gchar *fmt_amdgpu_pppp_mode(sysobj *obj, int fmt_opts) {
    gchar *val = fmt_amdgpu_val(obj, fmt_opts);
    if (!(fmt_opts & FMT_OPT_COMPLETE)) {
        gchar *c = strchr(val, ']');
        if (c) *(c+1) = 0;
    }
    return val;
}

static attr_tab pci_amdgpu_items[] = {
    { "pp_dpm_mclk", N_("memory clock frequency"), OF_NONE, fmt_amdgpu_clk, 0.5 },
    { "pp_dpm_sclk", N_("core clock frequency"), OF_NONE, fmt_amdgpu_clk, 0.5 },
    { "pp_dpm_pcie", N_("target PCI-Express operating mode"), OF_NONE, fmt_amdgpu_val, 0.5 },
    { "pp_sclk_od", N_("AMD OverDrive target core overclock"), OF_NONE, fmt_percent, 0.5 },
    { "pp_mclk_od", N_("AMD OverDrive target memory overclock"), OF_NONE, fmt_percent, 0.5 },
    { "pp_power_profile_mode", N_("power profile mode"), OF_NONE, fmt_amdgpu_pppp_mode, 0.5 },
    ATTR_TAB_LAST
};

vendor_list drm_card_vendors(sysobj *obj) {
    vendor_list vl = sysobj_vendors_from_fn(obj->path, "device");
    if (!vl)
        vl = sysobj_vendors_from_fn(obj->path, "device/of_node/compatible");

    /* monitors */
    sysobj_read(obj, FALSE);
    for(GSList *l = obj->data.childs; l; l = l->next) {
        gchar *c = l->data;
        if (g_str_has_prefix(c, "card") )
            vl = vendor_list_concat(vl, sysobj_vendors_from_fn(obj->path, c) );
    }

    return vl;
}

static sysobj_class cls_gpu[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "gpu:list", .pattern = ":/gpu", .flags = OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("list of discovered graphics processor devices"),
    .f_verify = gpu_list_verify, .f_format = gpu_list_format, .f_vendors = gpu_all_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "gpu", .pattern = ":/gpu/gpu*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("graphics device"),
    .v_lblnum = "gpu", .f_format = gpu_format_nice_name, .f_vendors = gpu_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "gpu:prop", .pattern = ":/gpu/gpu*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_lblnum_child = "gpu", .attributes = gpu_prop_items },

  { SYSOBJ_CLASS_DEF
    .tag = "drm:card", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("DRM-managed graphics device"),
    .f_verify = drm_card_verify, .f_format = drm_card_format, .f_vendors = drm_card_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:card:attr", .pattern = "/sys/devices/*/drm/card*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = drm_card_attr_verify, .f_format = drm_card_attr_format,
    .attributes = drm_items },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:link", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("DRM connection"), .s_node_format = "{{enabled}}{{; |status}}{{: |edid}}",
    .s_vendors_from_child = "edid", .f_verify = drm_conn_verify },
  { SYSOBJ_CLASS_DEF
    .tag = "drm:link:attr", .pattern = "/sys/devices/*/drm/card*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = drm_conn_attr_verify, .attributes = drm_conn_items, .f_vendors = edid_vendor },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:amdgpu", .pattern = "/sys/devices*/????:??:??.?/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = pci_amdgpu_items },
};

vendor_list gpu_vendors(sysobj *obj) {
    vendor_list vl = sysobj_vendors_from_fn(obj->path, "drm");
    /* if not drm, try other found gpu devices */
    if (!vl) /* found pci device with video controller class */
        vl = sysobj_vendors_from_fn(obj->path, "pci");
    if (!vl) /* found in platform devices */
        vl = sysobj_vendors_from_fn(obj->path, "device/of_node/compatible");
    if (!vl) /* found in the device tree */
        vl = sysobj_vendors_from_fn(obj->path, "of_node/compatible");
    return vl;
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
        gchar *dt_name = dtr_compat_decode(dt_compat_obj->data.any, dt_compat_obj->data.len, FALSE, fmt_opts);
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
        ansi_color = safe_ansi_color(ansi_color, TRUE);
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

    /* Currently handled by the vendor_get_shortest_name()
     * function well enough */
        /* nvidia PCI strings are pretty nice already,
         * just shorten the company name */
        // s->nice_name = g_strdup_printf("%s %s", "nVidia", device_str);

    if (strstr(vendor_str, "Intel")) {
        /* Intel Graphics may have very long names,
         * like "Intel Corporation Seventh Generation Something Core Something Something Integrated Graphics Processor Revision Ninety-four" */
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

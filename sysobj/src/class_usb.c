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
#include "sysobj_extras.h"
#include "util_usb.h"
#include "format_funcs.h"

#define SYSFS_USB "/sys/bus/usb"

static gchar *usb_format(sysobj *obj, int fmt_opts);
static gchar *usb_format_idcomp(sysobj *obj, int fmt_opts);
static gchar *usb_format_class(sysobj *obj, int fmt_opts);
static gboolean usb_verify_device(sysobj *obj) { return (obj) ? verify_usb_device(obj->name) : FALSE; }
static gboolean usb_verify_bus(sysobj *obj) { return (obj) ? verify_usb_bus(obj->name) : FALSE; }
static gboolean usb_verify_iface(sysobj *obj) { return (obj) ? verify_usb_interface(obj->name) : FALSE; }

static vendor_list usb_vendor_lookup(sysobj *obj);
static vendor_list usb_vendor_dev(sysobj *obj) { return sysobj_vendors_from_fn(obj->path, "idVendor"); }
static vendor_list usb_all_vendors(sysobj *obj);

#define usb_update_interval 10.0

static gchar *fmt_bcddevice(sysobj *obj, int fmt_opts) {
    if (obj->data.str) {
        uint32_t rev = strtol(obj->data.str, NULL, 10);
        return g_strdup_printf("%02d.%02d", rev / 100, rev % 100);
    }
    return simple_format(obj, fmt_opts);
}

static attr_tab usb_dev_items[] = {
    { "idVendor", N_("usb.ids-provided vendor name"), OF_HAS_VENDOR, usb_format_idcomp },
    { "idProduct", N_("usb.ids-provided product name"), OF_NONE, usb_format_idcomp },

    { "manufacturer", N_("device-provided vendor name"), OF_HAS_VENDOR },
    { "product",   N_("device-provided product name") },
    { "version",   N_("USB version") },
    { "speed",     N_("bitrate"), OF_NONE, fmt_megabitspersecond },
    { "removable", N_("device is removable or fixed") },
    { "urbnum",    N_("number of URBs submitted for the whole device"), OF_NONE, NULL, 0.2 },
    { "tx_lanes",  N_("transmit lanes"), OF_NONE, NULL, 0.5 },
    { "rx_lanes",  N_("receive lanes"), OF_NONE, NULL, 0.5 },
    { "bMaxPower", N_("maximum power consumption"), OF_NONE, fmt_milliampere },
    { "bcdDevice", N_("device revision"), OF_NONE, fmt_bcddevice },
    { "serial",    N_("serial number") },

    { "bDeviceClass", NULL, OF_NONE, usb_format_class },
    { "bDeviceSubClass", NULL, OF_NONE, usb_format_class },
    { "bDeviceProtocol", NULL, OF_NONE, usb_format_class },
    ATTR_TAB_LAST
};

static attr_tab usb_iface_items[] = {
    { "bInterfaceClass", NULL, OF_NONE, usb_format_class },
    { "bInterfaceSubClass", NULL, OF_NONE, usb_format_class },
    { "bInterfaceProtocol", NULL, OF_NONE, usb_format_class },
    ATTR_TAB_LAST
};

/*
 * usbN and usbNportM
 */

static sysobj_class cls_usb[] = {
  /* all under :/usb */
  { SYSOBJ_CLASS_DEF
    .tag = "usb", .pattern = ":/usb", .flags = OF_CONST | OF_HAS_VENDOR,
    .f_format = usb_format, .s_update_interval = usb_update_interval, .f_vendors = usb_all_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:device_list", .pattern = "/sys/bus/usb/devices", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .f_format = usb_format, .s_update_interval = usb_update_interval, .f_vendors = usb_all_vendors },

  { SYSOBJ_CLASS_DEF
    .tag = "usb:bus", .pattern = "/sys/devices*/usb*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .v_subsystem = "/sys/bus/usb", .s_node_format = "{{idVendor}}{{idProduct}}",
    .f_verify = usb_verify_bus, .f_vendors = usb_vendor_dev,
    .s_update_interval = usb_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:bus:attr", .pattern = "/sys/devices*/usb*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = usb_dev_items, .f_vendors = usb_vendor_lookup },

  { SYSOBJ_CLASS_DEF
    .tag = "usb:device", .pattern = "/sys/devices*/*-*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .v_subsystem = "/sys/bus/usb", .s_node_format = "{{idVendor}}{{idProduct}}",
    .f_verify = usb_verify_device, .f_vendors = usb_vendor_dev,
    .s_update_interval = usb_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:device:attr", .pattern = "/sys/devices*/*-*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = usb_dev_items, .v_subsystem_parent = "/sys/bus/usb",
    .f_vendors = usb_vendor_lookup },

  { SYSOBJ_CLASS_DEF
    .tag = "usb:iface", .pattern = "/sys/devices*/*-*:*.*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem = "/sys/bus/usb", .f_verify = usb_verify_iface },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:iface:attr", .pattern = "/sys/devices*/*-*:*.*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/usb", .attributes = usb_iface_items },
};

static sysobj_virt vol[] = {
    { .path = ":/usb/bus", .str = SYSFS_USB,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":/usb", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

static int usb_device_count() {
    int ret = 0;
    sysobj *obj = sysobj_new_from_fn("/sys/bus/usb/devices", NULL);
    GSList *childs = sysobj_children(obj, NULL, NULL, FALSE);
    GSList *l = childs;
    while(l) {
        sysobj *udc = sysobj_new_from_fn(obj->path, (gchar*)l->data);
        /* if (usb_verify_device(udc) || usb_verify_bus(udc) ) ret++; */
        /* should already have been verified */
        if (udc->cls) {
            if (SEQ(udc->cls->tag, "usb:device")
                || SEQ(udc->cls->tag, "usb:bus") ) {
                    ret++;
            }
        }
        sysobj_free(udc);
        l = l->next;
    }
    g_slist_free_full(childs, g_free);
    sysobj_free(obj);
    return ret;
}

static gchar *usb_format(sysobj *obj, int fmt_opts) {
    if (SEQ(obj->path, ":/usb")
        || SEQ(obj->path, "/sys/bus/usb/devices") ) {
        //TODO: handle fmt_opts
        gchar *ret = NULL;
        int c = usb_device_count();
        const char *fmt = ngettext("%d device", "%d devices", c);
        ret = g_strdup_printf(fmt, c);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static vendor_list usb_vendor_lookup(sysobj *obj) {
    if (obj->data.is_utf8) {
        gchar *vendor_str = sysobj_raw_from_printf(
            ":/lookup/usb.ids/%04lx/name", strtoul(obj->data.str, NULL, 16) );
        const Vendor *v = vendor_match(vendor_str, NULL);
        g_free(vendor_str);
        if (v)
            return vendor_list_append(NULL, v);
    }
    return NULL;
}

vendor_list usb_all_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    sysobj *lo = SEQ(obj->path, "/sys/bus/usb/devices")
        ? obj
        : sysobj_new_fast("/sys/bus/usb/devices");
    sysobj_read(lo, FALSE);
    GSList *childs = sysobj_children(lo, NULL, NULL, TRUE);
    for(GSList *l = childs; l; l = l->next) {
        if (verify_usb_device(l->data) || verify_usb_bus(l->data))
            ret = vendor_list_concat(ret, sysobj_vendors_from_fn(lo->path, l->data));
    }
    g_slist_free_full(childs, g_free);
    if (lo != obj)
        sysobj_free(lo);
    return ret;
}

static gchar *usb_format_class(sysobj *obj, int fmt_opts) {
    gchar *cls, *scls, *proto;
    if (SEQ(obj->cls->tag, "usb:device:attr") ) {
        cls = "bDeviceClass";
        scls = "bDeviceSubClass";
        proto = "bDeviceProtocol";
    } else if (SEQ(obj->cls->tag, "usb:iface:attr") ) {
        cls = "bInterfaceClass";
        scls = "bInterfaceSubClass";
        proto = "bInterfaceProtocol";
    } else
        return simple_format(obj, fmt_opts);

    gchar *ret = NULL;
    if (SEQ(obj->name, cls) ) {
        int class = strtol(obj->data.str, NULL, 16);
        gchar *cstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/usb.ids/C %02x/name", class);
        ret = (fmt_opts & FMT_OPT_PART)
            ? g_strdup(cstr ? cstr : _("Unknown"))
            : g_strdup_printf("[%02x] %s", class, cstr ? cstr : _("Unknown"));
        g_free(cstr);
    }
    if (SEQ(obj->name, scls) ) {
        int class = sysobj_uint32_from_printf(16, "%s/../%s,", obj->path, cls);
        int subclass = strtol(obj->data.str, NULL, 16);
        gchar *cstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/usb.ids/C %02x/%02x/name", class, subclass);
        ret = (fmt_opts & FMT_OPT_PART)
            ? g_strdup(cstr ? cstr : _("Unknown"))
            : g_strdup_printf("[%02x] %s", subclass, cstr ? cstr : _("Unknown"));
        g_free(cstr);
    }
    if (SEQ(obj->name, proto) ) {
        int class = sysobj_uint32_from_printf(16, "%s/../%s,", obj->path, cls);
        int subclass = sysobj_uint32_from_printf(16, "%s/../%s,", obj->path, scls);
        int proto = strtol(obj->data.str, NULL, 16);
        gchar *cstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/usb.ids/C %02x/%02x/%02x/name", class, subclass, proto);
        ret = (fmt_opts & FMT_OPT_PART)
            ? g_strdup(cstr ? cstr : _("Unknown"))
            : g_strdup_printf("[%02x] %s", proto, cstr ? cstr : _("Unknown"));
        g_free(cstr);
    }
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static gchar *usb_format_idcomp(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    if (SEQ(obj->name, "idVendor") ) {
        int vendor = strtol(obj->data.str, NULL, 16);
        gchar *vstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/usb.ids/%04x/name", vendor);
        gchar *ven_tag = vendor_match_tag(vstr, fmt_opts);
        if (ven_tag) {
            g_free(vstr);
            vstr = ven_tag;
        }
        ret = (fmt_opts & FMT_OPT_PART)
            ? g_strdup(vstr ? vstr : _("Unknown"))
            : g_strdup_printf("[%04x] %s", vendor, vstr ? vstr : _("Unknown"));
        g_free(vstr);
    }
    if (SEQ(obj->name, "idProduct") ) {
        int vendor = sysobj_uint32_from_fn(obj->path, "../idVendor", 16);
        int device = strtol(obj->data.str, NULL, 16);
        gchar *dstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/usb.ids/%04x/%04x/name", vendor, device);
        if (fmt_opts & FMT_OPT_PART) {
            if (!dstr) {
                dstr = sysobj_format_from_fn(obj->path, "../product", fmt_opts | FMT_OPT_PART);
                if (dstr) g_strstrip(dstr);
            }
            if (!dstr) {
                int cls = sysobj_uint32_from_fn(obj->path, "../bDeviceClass", 16);
                if (cls == 0xe0) {
                    dstr = sysobj_format_from_fn(obj->path, "../bDeviceProtocol", fmt_opts | FMT_OPT_PART);
                } else if (cls != 0 && cls < 0xef)
                    dstr = sysobj_format_from_fn(obj->path, "../bDeviceClass", fmt_opts | FMT_OPT_PART);
            }
            ret = g_strdup(dstr ? dstr : _("Device"));
        } else
            ret = g_strdup_printf("[%04x] %s", device, dstr ? dstr : _("Device"));
        g_free(dstr);
    }
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

void class_usb() {
    int i = 0;

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_usb); i++)
        class_add(&cls_usb[i]);
}

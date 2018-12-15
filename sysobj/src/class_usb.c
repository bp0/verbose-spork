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
#include "util_usb.h"
#include "format_funcs.h"

#define SYSFS_USB "/sys/bus/usb"

const gchar usb_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/usb/usb.ids/{vendor}/name\n"
    " :/usb/usb.ids/{vendor}/{device}/name\n\n"
    "Reference:\n"
    BULLET REFLINKT("<i>The Linux USB Project</i>'s usb.ids", "http://www.linux-usb.org/")
    "\n";

static gchar *usb_format(sysobj *obj, int fmt_opts);
static gchar *usb_format_idcomp(sysobj *obj, int fmt_opts);
static gboolean usb_verify_device(sysobj *obj) { return (obj) ? verify_usb_device(obj->name) : FALSE; }
static gchar *usb_format_device(sysobj *obj, int fmt_opts);
static gboolean usb_verify_bus(sysobj *obj) { return (obj) ? verify_usb_bus(obj->name) : FALSE; }
static gchar *usb_format_bus(sysobj *obj, int fmt_opts);

#define usb_update_interval 10.0

attr_tab usb_idcomp_items[] = {
    //TODO: labels
    { "idVendor", N_("usb.ids-provided vendor name"), OF_IS_VENDOR, NULL, -1 },
    { "idProduct", N_("usb.ids-provided product name"), OF_NONE, NULL, -1 },
    ATTR_TAB_LAST
};

static gchar *fmt_bcddevice(sysobj *obj, int fmt_opts) {
    if (obj->data.str) {
        uint32_t rev = strtol(obj->data.str, NULL, 10);
        return g_strdup_printf("%02d.%02d", rev / 100, rev % 100);
    }
    return simple_format(obj, fmt_opts);
}

attr_tab usb_dev_items[] = {
    { "manufacturer", N_("device-provided vendor name"), OF_IS_VENDOR, NULL, -1 },
    { "product",   N_("device-provided product name"), OF_NONE, NULL, -1 },
    { "version",   N_("USB version"), OF_NONE, NULL, -1 },
    { "speed",     N_("bitrate"), OF_NONE, fmt_megabitspersecond, -1 },
    { "removable", N_("device is removable or fixed"), OF_NONE, NULL, -1 },
    { "urbnum",    N_("number of URBs submitted for the whole device"), OF_NONE, NULL, 0.2 },
    { "tx_lanes",  N_("transmit lanes"), OF_NONE, NULL, 0.5 },
    { "rx_lanes",  N_("receive lanes"), OF_NONE, NULL, 0.5 },
    { "bMaxPower", N_("maximum power consumption"), OF_NONE, fmt_milliampere, -1 },
    { "bcdDevice", N_("device revision"), OF_NONE, fmt_bcddevice, -1 },
    { "serial",    N_("serial number"), OF_NONE, NULL, -1 },
    ATTR_TAB_LAST
};

/*
 * usbN and usbNportM
 */

static sysobj_class cls_usb[] = {
  /* all under :/usb */
  { SYSOBJ_CLASS_DEF
    .tag = "usb", .pattern = ":/usb*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = usb_format, .s_update_interval = usb_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:device_list", .pattern = "/sys/bus/usb/devices", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_format = usb_format, .s_update_interval = usb_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:bus", .pattern = "/sys/devices*/usb*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = usb_verify_bus,
    .f_format = usb_format_bus, .s_update_interval = usb_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:device", .pattern = "/sys/devices*/*-*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = usb_verify_device,
    .f_format = usb_format_device, .s_update_interval = usb_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:device_id", .pattern = "/sys/devices*/*-*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = usb_idcomp_items, .f_format = usb_format_idcomp },
  { SYSOBJ_CLASS_DEF
    .tag = "usb:device:attr", .pattern = "/sys/devices*/*-*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = usb_dev_items },
/*  { SYSOBJ_CLASS_DEF
    .tag = "usb:iface", .pattern = "/sys/devices*<<<<>>>>/*-*:*.*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = usb_verify_idcomp,
    .f_format = usb_format_idcomp, .s_update_interval = usb_update_interval }, */
/*  { SYSOBJ_CLASS_DEF
    .tag = "usb:iface_id", .pattern = "/sys/devices*<<<<>>>>/*-*:*.*<<<<>>>>/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .f_verify = usb_verify_idcomp,
    .f_format = usb_format_idcomp, .s_update_interval = usb_update_interval }, */

  { SYSOBJ_CLASS_DEF
    .tag = "usb.ids", .pattern = ":/usb/usb.ids", .flags = OF_CONST,
    .s_halp = usb_ids_reference_markup_text, .s_label = "usb.ids lookup virtual tree",
    .s_update_interval = 10.0 },
  { SYSOBJ_CLASS_DEF
    .tag = "usb.ids:id", .pattern = ":/usb/usb.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = usb_ids_reference_markup_text, .s_label = "usb.ids lookup result" },
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

util_usb_id *get_usb_id(gchar *dev_path) {
    util_usb_id *pid = g_new0(util_usb_id, 1);
    gchar path[64] = "";

    pid->vendor = sysobj_uint32_from_fn(dev_path, "idVendor", 16);
    pid->device = sysobj_uint32_from_fn(dev_path, "idProduct", 16);

    /* full first should cause all to be looked-up */
    sprintf(path, ":/usb/usb.ids/%04x/%04x", pid->vendor, pid->device);
    pid->device_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/usb/usb.ids/%04x", pid->vendor);
    pid->vendor_str = sysobj_raw_from_fn(path, "name");

    return pid;
}

static gchar *usb_format_idcomp(sysobj *obj, int fmt_opts) {
    gchar *pp = sysobj_parent_path(obj);
    util_usb_id *d = get_usb_id(pp);
    g_free(pp);

    gchar *ret = NULL;
    gchar *value_str = NULL;
    if (SEQ(obj->name, "idVendor") )
        value_str = d->vendor_str ? d->vendor_str : "Unknown";
    else if (SEQ(obj->name, "idProduct") )
        value_str = d->device_str ? d->device_str : "Device";
    if (value_str) {
        uint32_t value = strtol(obj->data.str, NULL, 16);
        ret = g_strdup_printf("[%04x] %s", value, value_str);
    }
    util_usb_id_free(d);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static gchar *usb_format_device(sysobj *obj, int fmt_opts) {
    util_usb_id *d = get_usb_id(obj->path);
    gchar *ret = NULL;
    if (d) {
        ret = g_strdup_printf("%s %s",
            d->vendor_str ? d->vendor_str : "Unknown",
            d->device_str ? d->device_str : "Device");
    }
    util_usb_id_free(d);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static gchar *usb_format_bus(sysobj *obj, int fmt_opts) {
    return usb_format_device(obj, fmt_opts);
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

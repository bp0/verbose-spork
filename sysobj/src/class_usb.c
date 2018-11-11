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

#define SYSFS_USB "/sys/bus/usb"

gchar* usb_log = NULL;
GSList *usb_id_list = NULL;

static gchar *usb_format(sysobj *obj, int fmt_opts);
static guint usb_flags(sysobj *obj);
static gchar *usb_messages(const gchar *path);
static double usb_update_interval(sysobj *obj);
static gboolean usb_verify_idcomp(sysobj *obj);
static gchar *usb_format_idcomp(sysobj *obj, int fmt_opts);
static gboolean usb_verify_device(sysobj *obj);
static gchar *usb_format_device(sysobj *obj, int fmt_opts);
static gboolean usb_verify_bus(sysobj *obj);
static gchar *usb_format_bus(sysobj *obj, int fmt_opts);

void class_usb_cleanup();

#define CLS_USB_FLAGS OF_GLOB_PATTERN | OF_CONST

/*
 * usbN and usbNportM
 */

static sysobj_class cls_usb[] = {
/*  { .tag = "usb/device_iface_id", .pattern = "/sys/devices*<<<<>>>>/*-*:*.*<<<<>>>>/*", .flags = CLS_USB_FLAGS,
    .f_verify = usb_verify_idcomp,
    .f_format = usb_format_idcomp, .f_flags = usb_flags, .f_update_interval = usb_update_interval }, */
/*  { .tag = "usb/device_iface", .pattern = "/sys/devices*<<<<>>>>/*-*:*.*", .flags = CLS_USB_FLAGS,
    .f_verify = usb_verify_idcomp,
    .f_format = usb_format_idcomp, .f_flags = usb_flags, .f_update_interval = usb_update_interval }, */
  { .tag = "usb/device_id", .pattern = "/sys/devices*/*-*/*", .flags = CLS_USB_FLAGS,
    .f_verify = usb_verify_idcomp,
    .f_format = usb_format_idcomp, .f_flags = usb_flags, .f_update_interval = usb_update_interval },
  { .tag = "usb/device", .pattern = "/sys/devices*/*-*", .flags = CLS_USB_FLAGS,
    .f_verify = usb_verify_device,
    .f_format = usb_format_device, .f_flags = usb_flags, .f_update_interval = usb_update_interval },
  { .tag = "usb/bus", .pattern = "/sys/devices*/usb*", .flags = CLS_USB_FLAGS,
    .f_verify = usb_verify_bus,
    .f_format = usb_format_bus, .f_flags = usb_flags, .f_update_interval = usb_update_interval },
  /* all under :/usb */
  { .tag = "usb", .pattern = ":/usb*", .flags = CLS_USB_FLAGS,
    .f_format = usb_format, .f_flags = usb_flags, .f_update_interval = usb_update_interval, .f_cleanup = class_usb_cleanup },
};

static sysobj_virt vol[] = {
    { .path = ":/usb/bus", .str = SYSFS_USB,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":/usb/_messages", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = usb_messages, .f_get_type = NULL },
    { .path = ":/usb", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

void usb_msg(char *fmt, ...) {
    gchar *buf, *tmp;
    va_list args;

    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);

    tmp = g_strdup_printf("%s%s\n", usb_log, buf);
    g_free(usb_log);
    usb_log = tmp;
}

static gchar *usb_messages(const gchar *path) {
    PARAM_NOT_UNUSED(path);
    return g_strdup(usb_log ? usb_log : "");
}

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
            if (!strcmp(udc->cls->tag, "usb/device")
                || !strcmp(udc->cls->tag, "usb/bus") ) {
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
    if (!strcmp(obj->path, ":/usb")
        || !strcmp(obj->path, "/sys/bus/usb/devices") ) {
        //TODO: handle fmt_opts
        gchar *ret = NULL;
        int c = usb_device_count();
        const char *fmt = ngettext("%d device", "%d devices", c);
        ret = g_strdup_printf(fmt, c);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static guint usb_flags(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return CLS_USB_FLAGS;
}

static double usb_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 10.0;
}

static util_usb_id *usb_find_id(gchar *address) {
    GSList *l = usb_id_list;
    while(l) {
        util_usb_id *d = (util_usb_id*)l->data;
        if (!g_strcmp0(address, d->address))
            return d;
        l = l->next;
    }
    return NULL;
}

/* [N-][P.P.P...][:C.I] */
static gboolean usb_verify_interface(sysobj *obj) {
    gchar *p = obj->name;
    if (!isdigit(*p)) return FALSE;
    int state = 0;
    while (*p != 0) {
        if ( isdigit(*p) ) { p++; continue; }
        switch(state) {
            case 0: if (*p == '-') { state = 1; } break;
            case 1:
                if (*p == ':') { state = 2; break; }
                if (*p != '.') { return FALSE; }
            case 2:
                if (*p != '.') { return FALSE; }
        }
        p++;
    }
    return TRUE;
}

/* usbN */
static gboolean usb_verify_bus(sysobj *obj) {
    return verify_lblnum(obj, "usb");
}

/* [N-][P.P.P...] */
static gboolean usb_verify_device(sysobj *obj) {
    gchar *p = obj->name;
    if (!isdigit(*p)) return FALSE;
    int state = 0;
    while (*p != 0) {
        if ( isdigit(*p) ) { p++; continue; }
        switch(state) {
            case 0: if (*p == '-') { state = 1; } break;
            case 1: if (*p != '.') { return FALSE; }
        }
        p++;
    }
    return TRUE;
}

static const gchar *usb_idcomps[] =
{ "idVendor", "idProduct" };

static gboolean usb_verify_idcomp(sysobj *obj) {
    int i = 0;
    for (i = 0; i < (int)G_N_ELEMENTS(usb_idcomps); i++ ) {
        if (!g_strcmp0(obj->name, usb_idcomps[i]) )
            return TRUE;
    }
    return FALSE;
}

static util_usb_id dev_not_found = { 0 };

static gchar *usb_format_idcomp(sysobj *obj, int fmt_opts) {
    gchar *pn = sysobj_parent_name(obj);
    util_usb_id *d = usb_find_id(pn);
    g_free(pn);
    if (!d) d = &dev_not_found;
    gchar *value_str = NULL;
    if (!g_strcmp0(obj->name, "idVendor") )
        value_str = d->vendor_str ? d->vendor_str : "Unknown";
    else if (!g_strcmp0(obj->name, "idProduct") )
        value_str = d->device_str ? d->device_str : "Device";
    if (value_str) {
        uint32_t value = strtol(obj->data.str, NULL, 16);
        return g_strdup_printf("[%04x] %s", value, value_str);
    }
    return simple_format(obj, fmt_opts);
}

static gchar *usb_format_device(sysobj *obj, int fmt_opts) {
    util_usb_id *d = usb_find_id(obj->name);
    if (d) {
        return g_strdup_printf("%s %s",
            d->vendor_str ? d->vendor_str : "Unknown",
            d->device_str ? d->device_str : "Device");
    }
    return simple_format(obj, fmt_opts);
}

static gchar *usb_format_bus(sysobj *obj, int fmt_opts) {
    return usb_format_device(obj, fmt_opts);
}

void class_usb_cleanup() {
    if (usb_id_list) {
        g_slist_free_full(usb_id_list, (GDestroyNotify)util_usb_id_free);
    }
    g_free(usb_log);
}

void usb_scan() {
    if (usb_id_list) return; /* already done */

    sysobj *obj = sysobj_new_from_fn(SYSFS_USB, "devices");

    if (!obj->exists) {
        usb_msg("usb device list not found at %s/devices", SYSFS_USB);
        sysobj_free(obj);
        return;
    }
    GSList *devs = sysobj_children(obj, NULL, NULL, TRUE);
    GSList *l = devs;
    while(l) {
        gchar *dev = (gchar*)l->data;
        sysobj *dobj = sysobj_new_from_fn(obj->path, dev);
        if (usb_verify_device(dobj) || usb_verify_bus(dobj) ) {
            util_usb_id *pid = g_new0(util_usb_id, 1);
            gchar *dev_path = g_strdup_printf("%s/%s", obj->path, dev);
            pid->address = g_strdup(dev);
            pid->vendor = sysobj_uint32_from_fn(dev_path, "idVendor", 16);
            pid->device = sysobj_uint32_from_fn(dev_path, "idProduct", 16);
            usb_id_list = g_slist_append(usb_id_list, pid);
            g_free(dev_path);
        }
        sysobj_free(dobj);
        l = l->next;
    }
    if (usb_id_list) {
        int count = g_slist_length(usb_id_list);
        int found = util_usb_ids_lookup_list(usb_id_list);
        if (found == -1)
            usb_msg("usb.ids file could not be read", found);
        else
            usb_msg("usb.ids matched for %d of %d devices", found, count);
    }
    sysobj_free(obj);
}

void class_usb() {
    int i = 0;

    usb_log = g_strdup("");

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_usb); i++) {
        class_add(&cls_usb[i]);
    }

    //TODO: should wait until first access to setup
    //other classes may not be ready
    usb_scan();
}

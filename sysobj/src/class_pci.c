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
#include "util_pci.h"

#define SYSFS_PCI "/sys/bus/pci"

const gchar pci_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/pci/pci.ids/{vendor}/name\n"
    " :/pci/pci.ids/{vendor}/{device}/name\n"
    " :/pci/pci.ids/{vendor}/{device}/{subvendor}/name\n"
    " :/pci/pci.ids/{vendor}/{device}/{subvendor}/{subdevice}/name\n\n"
    "Reference:\n"
    BULLET REFLINKT("<i>The PCI ID Repository</i>'s pci.ids", "https://pci-ids.ucw.cz")
    "\n";

static gchar *pci_ids_format(sysobj *obj, int fmt_opts);

static gchar *pci_format(sysobj *obj, int fmt_opts);
static guint pci_flags(sysobj *obj);
static double pci_update_interval(sysobj *obj);
static gboolean pci_verify_idcomp(sysobj *obj);
static gchar *pci_format_idcomp(sysobj *obj, int fmt_opts);
static gchar *pci_format_device(sysobj *obj, int fmt_opts);

static double pci_ids_update_interval(sysobj *obj);

#define CLS_PCI_FLAGS OF_GLOB_PATTERN | OF_CONST

static sysobj_class cls_pci[] = {
  /* all under :/pci */
  { SYSOBJ_CLASS_DEF
    .tag = "pci", .pattern = ":/pci*", .flags = CLS_PCI_FLAGS,
    .f_format = pci_format, .f_flags = pci_flags, .f_update_interval = pci_update_interval },

  { SYSOBJ_CLASS_DEF
    .tag = "pci:device_list", .pattern = "/sys/bus/pci/devices", .flags = CLS_PCI_FLAGS,
    .f_format = pci_format, .f_flags = pci_flags, .f_update_interval = pci_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:device", .pattern = "/sys/devices*/????:??:??.?", .flags = CLS_PCI_FLAGS,
    .f_format = pci_format_device, .f_flags = pci_flags, .f_update_interval = pci_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:device_id", .pattern = "/sys/devices*/????:??:??.?/*", .flags = CLS_PCI_FLAGS,
    .f_verify = pci_verify_idcomp,
    .f_format = pci_format_idcomp, .f_flags = pci_flags, .f_update_interval = pci_update_interval },

  { SYSOBJ_CLASS_DEF
    .tag = "pci.ids", .pattern = ":/pci/pci.ids", .flags = OF_CONST,
    .s_halp = pci_ids_reference_markup_text, .s_label = "pci.ids lookup virtual tree",
    .f_update_interval = pci_ids_update_interval, .f_format = pci_ids_format },
  { SYSOBJ_CLASS_DEF
    .tag = "pci.ids:id", .pattern = ":/pci/pci.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = pci_ids_reference_markup_text, .s_label = "pci.ids lookup result", .f_format = pci_ids_format },
};

static sysobj_virt vol[] = {
    { .path = ":/pci/bus", .str = SYSFS_PCI,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":/pci", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

static gboolean name_is_0x04(gchar *name) {
    gchar *p = name;
    while(isxdigit(*p)) p++;
    if (strlen(name) == 4 && p-name == 4 )
        return TRUE;
    return FALSE;
}

static gchar *pci_ids_format(sysobj *obj, int fmt_opts) {
    if ( name_is_0x04(obj->name) ) {
        gchar *ret = sysobj_raw_from_fn(obj->path, "name");
        if (ret)
            return ret;
    }
    return simple_format(obj, fmt_opts);
}

static int pci_device_count() {
    int ret = 0;
    sysobj *obj = sysobj_new_from_fn("/sys/bus/pci/devices", NULL);
    GSList *childs = sysobj_children(obj, NULL, NULL, FALSE);
    ret = g_slist_length(childs);
    g_slist_free_full(childs, g_free);
    sysobj_free(obj);
    return ret;
}

static gchar *pci_format(sysobj *obj, int fmt_opts) {
    if (SEQ(obj->path, ":/pci")
        || SEQ(obj->path, "/sys/bus/pci/devices") ) {
        //TODO: handle fmt_opts
        gchar *ret = NULL;
        int c = pci_device_count();
        const char *fmt = ngettext("%d device", "%d devices", c);
        ret = g_strdup_printf(fmt, c);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

static guint pci_flags(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return CLS_PCI_FLAGS;
}

static double pci_ids_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 10.0;
}

static double pci_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 10.0;
}

static const gchar *pci_idcomps[] =
{ "vendor", "device", "subsystem_vendor", "subsystem_device", "class" };

static gboolean pci_verify_idcomp(sysobj *obj) {
    int i = 0;
    for (i = 0; i < (int)G_N_ELEMENTS(pci_idcomps); i++ ) {
        if (SEQ(obj->name, pci_idcomps[i]) )
            return TRUE;
    }
    return FALSE;
}

util_pci_id *get_pci_id(gchar *dev_path) {
    util_pci_id *pid = g_new0(util_pci_id, 1);
    gchar path[64] = "";

    pid->vendor = sysobj_uint32_from_fn(dev_path, "vendor", 16);
    pid->device = sysobj_uint32_from_fn(dev_path, "device", 16);
    pid->sub_vendor = sysobj_uint32_from_fn(dev_path, "subsystem_vendor", 16);
    pid->sub_device = sysobj_uint32_from_fn(dev_path, "subsystem_device", 16);
    pid->dev_class = sysobj_uint32_from_fn(dev_path, "class", 16);

    /* full first should cause all to be looked-up */
    sprintf(path, ":/pci/pci.ids/%04x/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor, pid->sub_device);
    pid->sub_device_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/pci/pci.ids/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor);
    pid->sub_vendor_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/pci/pci.ids/%04x/%04x", pid->vendor, pid->device);
    pid->device_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/pci/pci.ids/%04x", pid->vendor);
    pid->vendor_str = sysobj_raw_from_fn(path, "name");

    //TODO: dev class

    return pid;
}

static gchar *pci_format_idcomp(sysobj *obj, int fmt_opts) {
    gchar *pp = sysobj_parent_path(obj);
    util_pci_id *d = get_pci_id(pp);
    g_free(pp);

    gchar *ret = NULL;
    gchar *value_str = NULL;
    if (SEQ(obj->name, "vendor") )
        value_str = d->vendor_str ? d->vendor_str : "Unknown";
    else if (SEQ(obj->name, "device") )
        value_str = d->device_str ? d->device_str : "Device";
    else if (SEQ(obj->name, "subsystem_vendor") )
        value_str = d->sub_vendor_str ? d->sub_vendor_str : "Unknown";
    else if (SEQ(obj->name, "subsystem_device") )
        value_str = d->sub_device_str ? d->sub_device_str : "Device";
    else if (SEQ(obj->name, "class") )
        value_str = d->dev_class_str ? d->dev_class_str : "Unknown";
    if (value_str) {
        uint32_t value = strtol(obj->data.str, NULL, 16);
        ret = g_strdup_printf("[%04x] %s", value, value_str);
    }
    util_pci_id_free(d);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static gchar *pci_format_device(sysobj *obj, int fmt_opts) {
    util_pci_id *d = get_pci_id(obj->path);
    gchar *ret = NULL;
    if (d) {
        ret = g_strdup_printf("%s %s",
            d->vendor_str ? d->vendor_str : "Unknown",
            d->device_str ? d->device_str : "Device");
    }
    util_pci_id_free(d);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

void class_pci() {
    int i = 0;

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_pci); i++) {
        class_add(&cls_pci[i]);
    }
}

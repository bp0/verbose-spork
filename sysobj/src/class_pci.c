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
#include "sysobj_extras.h"
#include "format_funcs.h"

#define SYSFS_PCI "/sys/bus/pci"

const gchar pci_ids_reference_markup_text[] =
    " Items are generated on-demand and cached.\n"
    "\n"
    " :/lookup/pci.ids/{vendor}/name\n"
    " :/lookup/pci.ids/{vendor}/{device}/name\n"
    " :/lookup/pci.ids/{vendor}/{device}/{subvendor}/name\n"
    " :/lookup/pci.ids/{vendor}/{device}/{subvendor}/{subdevice}/name\n\n"
    "Reference:\n"
    BULLET REFLINKT("<i>The PCI ID Repository</i>'s pci.ids", "https://pci-ids.ucw.cz")
    "\n";

static gchar *pci_format(sysobj *obj, int fmt_opts);
static gchar *pci_format_idcomp(sysobj *obj, int fmt_opts);
static gchar *pci_format_device(sysobj *obj, int fmt_opts);
static gchar *pci_format_class(sysobj *obj, int fmt_opts);

static vendor_list pci_vendor_lookup(sysobj *obj);
static vendor_list pci_vendor_dev(sysobj *obj);
static vendor_list pci_all_vendors(sysobj *obj);

#define pci_update_interval 6.0
#define pci_ids_update_interval 4.0

static attr_tab pci_idcomp_items[] = {
    { "vendor", N_("PCI-SIG-assigned vendor id"), OF_HAS_VENDOR },
    { "device", N_("vendor-specific device id") },
    { "subsystem_vendor", N_("PCI-assigned vendor id for sub-vendor"), OF_HAS_VENDOR },
    { "subsystem_device", N_("vendor-specific device id") },
    { "class", N_("device class"), OF_NONE, pci_format_class },
    ATTR_TAB_LAST
};

static attr_tab pci_var_items[] = {
    { "boot_vga", N_("is the primary graphics device"), OF_NONE, fmt_1yes0no, 60.0 },
    { "ari_enabled", N_("uses the alternative routing-ID interpretation"), OF_NONE, fmt_1yes0no, 60.0 },
    { "broken_parity_status", N_("the device is known to report false errors"), OF_NONE, fmt_1yes0no, 60.0 },
    { "enable", NULL, OF_NONE, fmt_1yes0no },
    ATTR_TAB_LAST
};

static attr_tab pcie_items[] = {
    //TODO: labels
    { "max_link_speed", NULL, OF_NONE, fmt_gigatransferspersecond, 4.0 },
    { "max_link_width", NULL, OF_NONE, fmt_lanes_x, 4.0 },
    { "current_link_speed", NULL, OF_NONE, fmt_gigatransferspersecond, 0.2 },
    { "current_link_width", NULL, OF_NONE, fmt_lanes_x, 0.2 },
    ATTR_TAB_LAST
};

static sysobj_class cls_pci[] = {
  /* all under :/pci */
  { SYSOBJ_CLASS_DEF
    .tag = "pci", .pattern = ":/pci", .flags = OF_CONST | OF_HAS_VENDOR,
    .f_format = pci_format, .s_update_interval = pci_update_interval, .f_vendors = pci_all_vendors },

  { SYSOBJ_CLASS_DEF
    .tag = "pci:device_list", .pattern = "/sys/bus/pci/devices", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .f_format = pci_format, .s_update_interval = pci_update_interval, .f_vendors = pci_all_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:device", .pattern = "/sys/devices*/????:??:??.?", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .f_format = pci_format_device, .s_update_interval = pci_update_interval, .f_vendors = pci_vendor_dev,
    .v_subsystem = "/sys/bus/pci" },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:device_id", .pattern = "/sys/devices*/????:??:??.?/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/pci",
    .attributes = pci_idcomp_items, .f_format = pci_format_idcomp, .f_vendors = pci_vendor_lookup },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:pcie", .pattern = "/sys/devices*/????:??:??.?/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/pci", .attributes = pcie_items },
  { SYSOBJ_CLASS_DEF
    .tag = "pci:var", .pattern = "/sys/devices*/????:??:??.?/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/pci", .attributes = pci_var_items },

  { SYSOBJ_CLASS_DEF
    .tag = "pci.ids", .pattern = ":/lookup/pci.ids", .flags = OF_CONST,
    .s_halp = pci_ids_reference_markup_text, .s_label = "pci.ids lookup virtual tree",
    .s_update_interval = pci_ids_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "pci.ids:id", .pattern = ":/lookup/pci.ids/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = pci_ids_reference_markup_text, .s_label = "pci.ids lookup result", .f_format = fmt_node_name },
  { SYSOBJ_CLASS_DEF
    .tag = "pci.ids:class", .pattern = ":/lookup/pci.ids/class", .flags = OF_CONST,
    .s_halp = pci_ids_reference_markup_text, .s_label = "pci.ids device class lookup results",
    .s_update_interval = pci_ids_update_interval },
  { SYSOBJ_CLASS_DEF
    .tag = "pci.ids:class:result", .pattern = ":/lookup/pci.ids/class/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = pci_ids_reference_markup_text, .s_label = "pci.ids device class lookup result",
    .s_update_interval = pci_ids_update_interval, .s_node_format = "{{class}}{{ :: |subclass}}{{ :: |progif}}" },
};

static sysobj_virt vol[] = {
    { .path = ":/pci/bus", .str = SYSFS_PCI,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST },
    { .path = ":/pci", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
};

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

static vendor_list pci_vendor_lookup(sysobj *obj) {
    if (obj->data.is_utf8) {
        gchar *vendor_str = sysobj_raw_from_printf(
            ":/lookup/pci.ids/%04lx/name", strtoul(obj->data.str, NULL, 16) );
        const Vendor *v  = vendor_match(vendor_str, NULL);
        g_free(vendor_str);
        if (v)
            return vendor_list_append(NULL, v);
    }
    return NULL;
}

static vendor_list pci_vendor_dev(sysobj *obj) {
    return vendor_list_concat(
        sysobj_vendors_from_fn(obj->path, "vendor"),
        sysobj_vendors_from_fn(obj->path, "subsystem_vendor") );
}

vendor_list pci_all_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    sysobj *lo = SEQ(obj->path, "/sys/bus/pci/devices")
        ? obj
        : sysobj_new_fast("/sys/bus/pci/devices");
    sysobj_read(lo, FALSE);
    GSList *childs = sysobj_children(lo, "????:??:??.?", NULL, TRUE);
    for(GSList *l = childs; l; l = l->next)
        ret = vendor_list_concat(ret, sysobj_vendors_from_fn(lo->path, l->data));
    g_slist_free_full(childs, g_free);
    if (lo != obj)
        sysobj_free(lo);
    return ret;
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
    sprintf(path, ":/lookup/pci.ids/%04x/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor, pid->sub_device);
    pid->sub_device_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/lookup/pci.ids/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor);
    pid->sub_vendor_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/lookup/pci.ids/%04x/%04x", pid->vendor, pid->device);
    pid->device_str = sysobj_raw_from_fn(path, "name");
    sprintf(path, ":/lookup/pci.ids/%04x", pid->vendor);
    pid->vendor_str = sysobj_raw_from_fn(path, "name");

    //TODO: dev class

    return pid;
}

static gchar *pci_format_class(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    uint32_t value = strtol(obj->data.str, NULL, 16);
    gchar *value_str = sysobj_format_from_printf(fmt_opts, ":/lookup/pci.ids/class/%06x", value & 0xffffff);
    if (value_str) {
        ret = g_strdup_printf("[%04x] %s", value, value_str);
        g_free(value_str);
    }
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
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
        vendor_list vl = sysobj_vendors(obj);
        const Vendor *v = vl ? vl->data : NULL;
        if (v) {
            gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
            tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, fmt_opts);
            ret = g_strdup_printf("%s %s",
                ven_tag, d->device_str ? d->device_str : "Device");
            g_free(ven_tag);
        } else {
            ret = g_strdup_printf("%s %s",
                d->vendor_str ? d->vendor_str : "Unknown",
                d->device_str ? d->device_str : "Device");
        }
        vendor_list_free(vl);
    }
    util_pci_id_free(d);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

void class_pci() {
    int i = 0;

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_pci); i++)
        class_add(&cls_pci[i]);
}

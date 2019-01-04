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
#include "format_funcs.h"

static gchar *scsi_format(sysobj *obj, int fmt_opts);
static gchar *scsi_vendor_format(sysobj *obj, int fmt_opts);
static gchar *scsi_dev_list_format(sysobj *obj, int fmt_opts);
vendor_list scsi_all_vendors(sysobj *obj);
static vendor_list scsi_dev_vendors(sysobj *obj);

static attr_tab scsi_items[] = {
    { "model", NULL, OF_HAS_VENDOR },
    { "vendor", NULL, OF_HAS_VENDOR, scsi_vendor_format },
    ATTR_TAB_LAST
};

static sysobj_class cls_scsi[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "scsi:dev_list", .pattern = "/sys/bus/scsi/devices", .flags = OF_CONST | OF_HAS_VENDOR,
    .f_format = scsi_dev_list_format, .f_vendors = scsi_all_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "scsi:dev", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .v_subsystem = "/sys/bus/scsi", .s_node_format = "{{@vendors!vendor}}{{model}}", .f_vendors = scsi_dev_vendors },
  { SYSOBJ_CLASS_DEF
    .tag = "scsi:dev:attr", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/scsi", .attributes = scsi_items },

  { SYSOBJ_CLASS_DEF
    .tag = "scsi_disk", .pattern = "/sys/devices/*/scsi_disk/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_vendors_from_child = "device",
    .v_subsystem = "/sys/class/scsi_disk", .s_node_format = "{{device}}" },

  { SYSOBJ_CLASS_DEF
    .tag = "scsi_host", .pattern = "/sys/devices/*/scsi_host/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_vendors_from_child = "vendor",
    .v_subsystem = "/sys/class/scsi_host", .s_node_format = "{{@vendors!vendor}}{{model}}" },
  { SYSOBJ_CLASS_DEF
    .tag = "scsi_host:attr", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/class/scsi_host", .attributes = scsi_items },

  { SYSOBJ_CLASS_DEF
    .tag = "scsi_generic", .pattern = "/sys/devices/*/scsi_generic/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .s_vendors_from_child = "device",
    .v_subsystem = "/sys/class/scsi_generic", .s_node_format = "{{device}}" },
};

vendor_list scsi_all_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    sysobj *lo = SEQ(obj->path, "/sys/bus/scsi/devices")
        ? obj
        : sysobj_new_fast("/sys/bus/scsi/devices");
    sysobj_read(lo, FALSE);
    GSList *childs = sysobj_children(lo, NULL, NULL, TRUE);
    for(GSList *l = childs; l; l = l->next)
        ret = vendor_list_concat(ret, sysobj_vendors_from_fn(lo->path, l->data));
    g_slist_free_full(childs, g_free);
    if (lo != obj)
        sysobj_free(lo);
    return ret;
}

static vendor_list scsi_dev_vendors(sysobj *obj) {
    return vendor_list_concat(
        sysobj_vendors_from_fn(obj->path, "vendor"),
        sysobj_vendors_from_fn(obj->path, "model") );
}

static int scsi_device_count() {
    int ret = 0;
    sysobj *obj = sysobj_new_from_fn("/sys/bus/scsi/devices", NULL);
    sysobj_read(obj, FALSE);
    ret = g_slist_length(obj->data.childs);
    sysobj_free(obj);
    return ret;
}

static gchar *scsi_dev_list_format(sysobj *obj, int fmt_opts) {
    //TODO: handle fmt_opts
    gchar *ret = NULL;
    int c = scsi_device_count();
    const char *fmt = ngettext("%d device", "%d devices", c);
    ret = g_strdup_printf(fmt, c);
    return ret;
}

static gchar *scsi_vendor_format(sysobj *obj, int fmt_opts) {
    if (obj->exists && obj->data.str) {
        gchar *val = auto_free(g_strdup(obj->data.str));
        g_strchomp(val);
        if (SEQ(val, "ATA"))
            return format_as_junk_value(val, fmt_opts);
    }
    return fmt_vendor_name_to_tag(obj, fmt_opts);
}

void class_scsi() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_scsi); i++)
        class_add(&cls_scsi[i]);
}

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

static gchar *any_bus_format(sysobj *obj, int fmt_opts);
static gchar *any_bus_dev_list_format(sysobj *obj, int fmt_opts);
vendor_list any_bus_all_vendors(sysobj *obj);

static sysobj_class cls_any_bus[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "subsystem:bus", .pattern = "/sys/bus/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_BLAST,
    .v_parent_path_suffix = "/sys/bus", .f_format = any_bus_format, .s_label = N_("sysfs sub-system"),
    .s_update_interval = UPDATE_INTERVAL_NEVER },
  { SYSOBJ_CLASS_DEF
    .tag = "subsystem:bus:dev_list", .pattern = "/sys/bus/*/devices", .flags = OF_GLOB_PATTERN | OF_CONST | OF_BLAST | OF_HAS_VENDOR,
    .f_format = any_bus_dev_list_format, .f_vendors = any_bus_all_vendors,
    .s_update_interval = 6.0 },
};

vendor_list any_bus_all_vendors(sysobj *obj) {
    vendor_list ret = NULL;
    sysobj_read(obj, FALSE);
    GSList *childs = sysobj_children(obj, NULL, NULL, TRUE);
    for(GSList *l = childs; l; l = l->next)
        ret = vendor_list_concat(ret, sysobj_vendors_from_fn(obj->path, l->data));
    g_slist_free_full(childs, g_free);
    return ret;
}

static int any_bus_device_count(gchar *bus_name) {
    int ret = 0;
    sysobj *obj = sysobj_new_from_printf("/sys/bus/%s/devices", bus_name);
    sysobj_read(obj, FALSE);
    ret = g_slist_length(obj->data.childs);
    sysobj_free(obj);
    return ret;
}

static gchar *any_bus_dev_list_format(sysobj *obj, int fmt_opts) {
    //TODO: handle fmt_opts
    gchar *ret = NULL;
    gchar *pn = sysobj_parent_name(obj);
    int c = any_bus_device_count(pn);
    const char *fmt = ngettext("%d device", "%d devices", c);
    ret = g_strdup_printf(fmt, c);
    g_free(pn);
    return ret;
}

static gchar *any_bus_format(sysobj *obj, int fmt_opts) {
    gchar *dev_count = sysobj_format_from_fn(obj->path, "devices", fmt_opts | FMT_OPT_PART);
    gchar *ret = g_strdup_printf("%s:%s (%s)", _("bus"), obj->name, dev_count);
    g_free(dev_count);
    return ret;
}

static gchar *any_bus_dev_format(sysobj *obj, int fmt_opts) {
    if (SEQ(obj->cls->tag, "any_bus:dev") ) {
        gchar *vendor = sysobj_format_from_fn(obj->path, "vendor", fmt_opts | FMT_OPT_OR_NULL);
        gchar *model = sysobj_format_from_fn(obj->path, "model", fmt_opts | FMT_OPT_OR_NULL);
        gchar *ret = NULL;
        if (vendor)
            ret = appf(ret, "%s", vendor);
        if (model)
            ret = appf(ret, "%s", model);
        if (ret)
            return ret;
    }
    return simple_format(obj, fmt_opts);
}

void class_any_bus() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_any_bus); i++)
        class_add(&cls_any_bus[i]);
}

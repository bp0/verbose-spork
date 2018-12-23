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

/* Generator for ids from the usb.ids file (http://www.linux-usb.org/usb.ids)
 *  :/usb/usb.ids
 *
 *
 * Items are generated on-demand and cached.
 *
 * :/usb/usb.ids/<vendor>/name
 * :/usb/usb.ids/<vendor>/<device>/name
 *
 * TODO: usb device and interface classes
 *
 */
#include "sysobj.h"
#include "util_usb.h"

#define SYSFS_USB "/sys/bus/usb"

static void gen_usb_ids_cache_item(util_usb_id *pid) {
    gchar buff[128] = "";
    int dev_symlink_flags = VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN;
    gchar *devpath = g_strdup_printf(SYSFS_USB "/devices/%s", pid->address);
    if (pid->vendor) {
        sprintf(buff, ":/usb/usb.ids/%04x", pid->vendor);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->vendor_str)
            sysobj_virt_add_simple(buff, "name", pid->vendor_str, VSO_TYPE_STRING );
        if (pid->address)
            sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );
    }
    if (pid->device) {
        sprintf(buff, ":/usb/usb.ids/%04x/%04x", pid->vendor, pid->device);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->device_str)
            sysobj_virt_add_simple(buff, "name", pid->device_str, VSO_TYPE_STRING );
        if (pid->address)
            sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );
    }
    g_free(devpath);
}

#define usb_msg(...) /* fprintf (stderr, __VA_ARGS__) */

static void usb_scan() {
    GSList *usb_id_list = NULL, *l = NULL;

    sysobj *obj = sysobj_new_from_fn(SYSFS_USB, "devices");

    if (!obj->exists) {
        usb_msg("usb device list not found at %s/devices\n", SYSFS_USB);
        sysobj_free(obj);
        return;
    }

    GSList *devs = sysobj_children(obj, NULL, NULL, TRUE);
    for(l = devs; l; l = l->next) {
        gchar *dev = (gchar*)l->data;
        if (verify_usb_device(dev) || verify_usb_bus(dev) ) {
            util_usb_id *pid = g_new0(util_usb_id, 1);
            gchar *dev_path = g_strdup_printf("%s/%s", obj->path, dev);
            pid->address = g_strdup(dev);
            pid->vendor = sysobj_uint32_from_fn(dev_path, "idVendor", 16);
            pid->device = sysobj_uint32_from_fn(dev_path, "idProduct", 16);
            usb_id_list = g_slist_append(usb_id_list, pid);
            g_free(dev_path);
        }
    }
    g_slist_free_full(devs, (GDestroyNotify)g_free);

    if (usb_id_list) {
        int count = g_slist_length(usb_id_list);
        int found = util_usb_ids_lookup_list(usb_id_list);
        for(l = usb_id_list; l; l = l->next) {
            gen_usb_ids_cache_item((util_usb_id*)l->data);
        }

        if (found == -1)
            usb_msg("usb.ids file could not be read\n");
        else
            usb_msg("usb.ids matched for %d of %d devices\n", found, count);
    }
    sysobj_free(obj);
    g_slist_free_full(usb_id_list, (GDestroyNotify)util_usb_id_free);
}

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

static gboolean name_is_0x04(gchar *name) {
    if (!name) return FALSE;
    if (  isxdigit(name[0])
        && isxdigit(name[1])
        && isxdigit(name[2])
        && isxdigit(name[3])
        && name[4] == 0 )
        return TRUE;
    return FALSE;
}

static gchar *gen_usb_ids_lookup_value(const gchar *path) {
    if (!path) return NULL;
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "usb.ids") )
        return g_strdup("*");

    util_usb_id *pid = g_new0(util_usb_id, 1);
    int32_t name_id = -1;
    if ( name_is_0x04(name) )
        name_id = strtol(name, NULL, 16);

    int mc = sscanf(path, ":/usb/usb.ids/%04x/%04x", &pid->vendor, &pid->device);
    gchar *verify = NULL;
    switch(mc) {
        case 2:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/usb/usb.ids/%04x/%04x/%s", pid->vendor, pid->device, name)
                : g_strdup_printf(":/usb/usb.ids/%04x/%04x", pid->vendor, pid->device);
        case 1:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/usb/usb.ids/%04x/%s", pid->vendor, name)
                : g_strdup_printf(":/usb/usb.ids/%04x", pid->vendor);

            if (strcmp(path, verify) != 0) {
                util_usb_id_free(pid);
                return NULL;
            }

            int ok = util_usb_ids_lookup(pid);
            gen_usb_ids_cache_item(pid);
            break;
        case 0:
            util_usb_id_free(pid);
            return NULL;
    }

    if (name_id != -1) {
        util_usb_id_free(pid);
        return "*";
    }

    gchar *ret = NULL;
    if (SEQ(name, "name") ) {
        switch(mc) {
            case 1: ret = g_strdup(pid->vendor_str); break;
            case 2: ret = g_strdup(pid->device_str); break;
        }
    }
    util_usb_id_free(pid);
    return ret;
}

static int gen_usb_ids_lookup_type(const gchar *path) {
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "usb.ids") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN;

    if (SEQ(name, "name") )
        return VSO_TYPE_STRING;

    if (name_is_0x04(name) )
        return VSO_TYPE_DIR;

    return VSO_TYPE_NONE;
}

void gen_usb_ids() {
    sysobj_virt *vo = sysobj_virt_new();
    vo->path = g_strdup(":/usb/usb.ids");
    vo->str = g_strdup("*");
    vo->type = VSO_TYPE_DIR | VSO_TYPE_DYN;
    vo->f_get_data = gen_usb_ids_lookup_value;
    vo->f_get_type = gen_usb_ids_lookup_type;
    sysobj_virt_add(vo);

    /* this is not required, but it is more effecient to
     * look the whole list up at once */
    usb_scan();
}

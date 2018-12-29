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

/* Generator for ids from the pci.ids file (https://pci-ids.ucw.cz/)
 *  :/lookup/pci.ids
 *
 *
 * Items are generated on-demand and cached.
 *
 * :/lookup/pci.ids/<vendor>/name
 * :/lookup/pci.ids/<vendor>/<device>/name
 * :/lookup/pci.ids/<vendor>/<device>/<subvendor>/name
 * :/lookup/pci.ids/<vendor>/<device>/<subvendor>/<subdevice>/name
 *
 * TODO: pci device classes
 *
 */
#include "sysobj.h"
#include "util_pci.h"

#define SYSFS_PCI "/sys/bus/pci"

static void gen_pci_ids_cache_item(util_pci_id *pid) {
    gchar buff[128] = "";
    int dev_symlink_flags = VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN;
    gchar *devpath = g_strdup_printf(SYSFS_PCI "/devices/%s", pid->address);
    if (pid->vendor) {
        sprintf(buff, ":/lookup/pci.ids/%04x", pid->vendor);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->vendor_str)
            sysobj_virt_add_simple(buff, "name", pid->vendor_str, VSO_TYPE_STRING );
        if (pid->address)
            sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );
    }
    if (pid->device) {
        sprintf(buff, ":/lookup/pci.ids/%04x/%04x", pid->vendor, pid->device);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->device_str)
            sysobj_virt_add_simple(buff, "name", pid->device_str, VSO_TYPE_STRING );
        if (pid->address)
            sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );
    }
    if (pid->sub_vendor) {
        sprintf(buff, ":/lookup/pci.ids/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->sub_vendor_str)
            sysobj_virt_add_simple(buff, "name", pid->sub_vendor_str, VSO_TYPE_STRING );
        if (pid->address)
            sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );

        /* also cache as vendor */
        if (pid->sub_vendor_str) {
            sprintf(buff, ":/lookup/pci.ids/%04x", pid->sub_vendor);
            sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
            sysobj_virt_add_simple(buff, "name", pid->sub_vendor_str, VSO_TYPE_STRING );
        }
    }
    if (pid->sub_device) {
        sprintf(buff, ":/lookup/pci.ids/%04x/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor, pid->sub_device);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->sub_device_str)
            sysobj_virt_add_simple(buff, "name", pid->sub_device_str, VSO_TYPE_STRING );
        if (pid->address)
            sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );
    }
    /* class */
    sprintf(buff, ":/lookup/pci.ids/class/%06x", pid->dev_class);
    #define OR_EMPTY(s) (s ? s : "")
    sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
    sysobj_virt_add_simple(buff, "class", OR_EMPTY(pid->dev_class_str), VSO_TYPE_STRING );
    sysobj_virt_add_simple(buff, "subclass", OR_EMPTY(pid->dev_subclass_str), VSO_TYPE_STRING );
    sysobj_virt_add_simple(buff, "progif", OR_EMPTY(pid->dev_progif_str), VSO_TYPE_STRING );
    if (pid->address)
        sysobj_virt_add_simple(buff, pid->address, devpath, dev_symlink_flags );
    g_free(devpath);
}

#define pci_msg(...) /* fprintf (stderr, __VA_ARGS__) */

static void pci_scan() {
    GSList *pci_id_list = NULL, *l = NULL;

    sysobj *obj = sysobj_new_from_fn(SYSFS_PCI, "devices");

    if (!obj->exists) {
        pci_msg("pci device list not found at %s/devices\n", SYSFS_PCI);
        sysobj_free(obj);
        return;
    }

    GSList *devs = sysobj_children(obj, NULL, NULL, TRUE);
    for(l = devs; l; l = l->next) {
        gchar *dev = (gchar*)l->data;
        if (verify_pci_addy(dev) ) {
            util_pci_id *pid = g_new0(util_pci_id, 1);
            gchar *dev_path = g_strdup_printf("%s/%s", obj->path, dev);
            pid->address = g_strdup(dev);
            pid->vendor = sysobj_uint32_from_fn(dev_path, "vendor", 16);
            pid->device = sysobj_uint32_from_fn(dev_path, "device", 16);
            pid->sub_vendor = sysobj_uint32_from_fn(dev_path, "subsystem_vendor", 16);
            pid->sub_device = sysobj_uint32_from_fn(dev_path, "subsystem_device", 16);
            pid->dev_class = sysobj_uint32_from_fn(dev_path, "class", 16);
            pci_id_list = g_slist_append(pci_id_list, pid);
            g_free(dev_path);
        }
    }
    g_slist_free_full(devs, (GDestroyNotify)g_free);

    if (pci_id_list) {
        int count = g_slist_length(pci_id_list);
        int found = util_pci_ids_lookup_list(pci_id_list);
        for(l = pci_id_list; l; l = l->next) {
            gen_pci_ids_cache_item((util_pci_id*)l->data);
        }

        if (found == -1)
            pci_msg("pci.ids file could not be read\n");
        else
            pci_msg("pci.ids matched for %d of %d devices\n", found, count);
    }
    sysobj_free(obj);
    g_slist_free_full(pci_id_list, (GDestroyNotify)util_pci_id_free);
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

static gboolean name_is_0x06(gchar *name) {
    if (!name) return FALSE;
    if (  isxdigit(name[0])
        && isxdigit(name[1])
        && isxdigit(name[2])
        && isxdigit(name[3])
        && isxdigit(name[4])
        && isxdigit(name[5])
        && name[6] == 0 )
        return TRUE;
    return FALSE;
}

static gchar *gen_pci_ids_lookup_value(const gchar *path) {
    if (!path) return NULL;
    gchar name[16] = "";
    buff_basename(path, name, 15);

    pci_msg("gen_pci_ids_lookup_value(): %s\n", path);

    if (SEQ(name, "pci.ids") )
        return g_strdup("*");

    util_pci_id *pid = g_new0(util_pci_id, 1);
    int32_t name_id = -1;
    if ( name_is_0x04(name) )
        name_id = strtol(name, NULL, 16);

    int mc = sscanf(path, ":/lookup/pci.ids/%04x/%04x/%04x/%04x", &pid->vendor, &pid->device, &pid->sub_vendor, &pid->sub_device);
    gchar *verify = NULL;
    switch(mc) {
        case 4:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/pci.ids/%04x/%04x/%04x/%04x/%s", pid->vendor, pid->device, pid->sub_vendor, pid->sub_device, name)
                : g_strdup_printf(":/lookup/pci.ids/%04x/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor, pid->sub_device);
        case 3:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/pci.ids/%04x/%04x/%04x/%s", pid->vendor, pid->device, pid->sub_vendor, name)
                : g_strdup_printf(":/lookup/pci.ids/%04x/%04x/%04x", pid->vendor, pid->device, pid->sub_vendor);
        case 2:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/pci.ids/%04x/%04x/%s", pid->vendor, pid->device, name)
                : g_strdup_printf(":/lookup/pci.ids/%04x/%04x", pid->vendor, pid->device);
        case 1:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/pci.ids/%04x/%s", pid->vendor, name)
                : g_strdup_printf(":/lookup/pci.ids/%04x", pid->vendor);

            if (strcmp(path, verify) != 0) {
                util_pci_id_free(pid);
                return NULL;
            }

            int ok = util_pci_ids_lookup(pid);
            gen_pci_ids_cache_item(pid);
            break;
        case 0:
            util_pci_id_free(pid);
            return NULL;
    }

    if (name_id != -1) {
        util_pci_id_free(pid);
        return "*";
    }

    gchar *ret = NULL;
    if (SEQ(name, "name") ) {
        switch(mc) {
            case 1: ret = g_strdup(pid->vendor_str); break;
            case 2: ret = g_strdup(pid->device_str); break;
            case 3: ret = g_strdup(pid->sub_vendor_str); break;
            case 4: ret = g_strdup(pid->sub_device_str); break;
        }
    }
    util_pci_id_free(pid);
    return ret;
}

static int gen_pci_ids_lookup_type(const gchar *path) {
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "pci.ids") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST;

    if (SEQ(name, "name") )
        return VSO_TYPE_STRING;

    if (name_is_0x04(name) )
        return VSO_TYPE_DIR;

    return VSO_TYPE_NONE;
}

static gchar *gen_pci_ids_lookup_class_value(const gchar *path) {
    if (!path) return NULL;
    gchar name[16] = "";
    buff_basename(path, name, 15);

    pci_msg("gen_pci_ids_lookup_class_value(): %s\n", path);

    if (SEQ(path, ":/lookup/pci.ids/class") )
        return g_strdup("*");

    util_pci_id *pid = g_new0(util_pci_id, 1);
    int32_t name_id = -1;
    if ( name_is_0x06(name) )
        name_id = strtol(name, NULL, 16);

    int mc = sscanf(path, ":/lookup/pci.ids/class/%06x", &pid->dev_class);
    gchar *verify = NULL;
    switch(mc) {
        case 1:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/pci.ids/class/%06x/%s", pid->dev_class & 0xffffff, name)
                : g_strdup_printf(":/lookup/pci.ids/class/%06x", pid->dev_class & 0xffffff);

            if (strcmp(path, verify) != 0) {
                util_pci_id_free(pid);
                return NULL;
            }

            int ok = util_pci_ids_lookup(pid);
            gen_pci_ids_cache_item(pid);
            break;
        case 0:
            util_pci_id_free(pid);
            return NULL;
    }

    gchar *ret = NULL;
    if (SEQ(name, "class") )
        ret = g_strdup(pid->dev_class_str);
    if (SEQ(name, "subclass") )
        ret = g_strdup(pid->dev_subclass_str);
    if (SEQ(name, "progid") )
        ret = g_strdup(pid->dev_progif_str);

    util_pci_id_free(pid);
    return ret;
}

static int gen_pci_ids_lookup_class_type(const gchar *path) {
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(path, ":/lookup/pci.ids/class") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST;

    if (SEQ(name, "class")
        || SEQ(name, "subclass")
        || SEQ(name, "progif") )
        return VSO_TYPE_STRING;

    if (name_is_0x06(name) )
        return VSO_TYPE_DIR;

    return VSO_TYPE_NONE;
}

static sysobj_virt vol[] = {
    { .path = ":/lookup/pci.ids/class", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = gen_pci_ids_lookup_class_value, .f_get_type = gen_pci_ids_lookup_class_type },
    { .path = ":/lookup/pci.ids", .str = "*",
      .type =  VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = gen_pci_ids_lookup_value, .f_get_type = gen_pci_ids_lookup_type },
};

void gen_pci_ids() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    /* this is not required, but it is more effecient to
     * look the whole list up at once */
    pci_scan();
}

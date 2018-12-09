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

/* Generator for GPU information tree
 *  :/gpu
 */
#include "sysobj.h"
#include "sysobj_foreach.h"
#include "util_pci.h"
#include "vendor.h"

#if 0
static void gpu_make_nice_name(sysobj *gpu_obj) {
    static const char unk_v[] = "Unknown"; /* do not...    */
    static const char unk_d[] = "Device";  /* ...translate */
    const char *vendor_str = s->vendor_str;
    const char *device_str = s->device_str;
    if (!vendor_str)
        vendor_str = unk_v;
    if (!device_str)
        device_str = unk_d;

    /* try and a get a "short name" for the vendor */
    vendor_str = vendor_get_shortest_name(vendor_str);

    /* These two former special cases are currently handled by the vendor_get_shortest_name()
     * function well enough, but the notes are preserved here. */
        /* nvidia PCI strings are pretty nice already,
         * just shorten the company name */
        // s->nice_name = g_strdup_printf("%s %s", "nVidia", device_str);
        /* Intel Graphics may have very long names, like "Intel Corporation Seventh Generation Something Core Something Something Integrated Graphics Processor Revision Ninety-four"
         * but for now at least shorten "Intel Corporation" to just "Intel" */
        // s->nice_name = g_strdup_printf("%s %s", "Intel", device_str);

    if (strstr(vendor_str, "AMD")) {
        /* AMD PCI strings are crazy stupid because they use the exact same
         * chip and device id for a zillion "different products" */
        char *full_name = strdup(device_str);
        /* Try and shorten it to the chip code name only, at least */
        char *b = strchr(full_name, '[');
        if (b) *b = '\0';
        s->nice_name = g_strdup_printf("%s %s", "AMD/ATI", g_strstrip(full_name));
        free(full_name);
    } else {
        /* nothing nicer */
        s->nice_name = g_strdup_printf("%s %s", vendor_str, device_str);
    }
}
#endif

static void find_drm_cards() {
    sysobj *drm_obj = sysobj_new_from_fn("/sys/class/drm", NULL);
    if (drm_obj->exists) {
        sysobj_read(drm_obj, FALSE);
        for(GSList *l = drm_obj->data.childs; l; l = l->next) {
            sysobj *card_obj = sysobj_new_from_fn(drm_obj->path, (gchar*)l->data);
            if (verify_lblnum(card_obj, "card") ) {
                gchar *gpu_id = g_strdup_printf("drm-%s", card_obj->name_req);
                sysobj_virt_add_simple(":/gpu/found",  gpu_id, card_obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
                g_free(gpu_id);
            }
            sysobj_free(card_obj);
        }
    }
    sysobj_free(drm_obj);
}

/* find all PCI devices with device class 0x03, display controller,
 * (sysfs pci class attribute = 0x030000 - 0x03ffff)
 * - or -
 * class 00 subclass 01, VGA compatible unclassified device
 * (sysfs pci class attribute = 0x000100 - 0x0001ff)
 */
static void find_pci_vga_devices() {
    sysobj *drm_obj = sysobj_new_from_fn("/sys/bus/pci/devices", NULL);
    if (drm_obj->exists) {
        sysobj_read(drm_obj, FALSE);
        for(GSList *l = drm_obj->data.childs; l; l = l->next) {
            sysobj *card_obj = sysobj_new_from_fn(drm_obj->path, (gchar*)l->data);
            if (verify_pci_device(card_obj->name) ) {
                gchar *class_str = sysobj_raw_from_fn(card_obj->path, "class");
                if (class_str) {
                    int pci_class = strtol(class_str, NULL, 16);
                    if ( (pci_class >= 0x30000 && pci_class <= 0x3ffff)
                        || (pci_class >= 0x000100 && pci_class <= 0x0001ff) ) {
                        gchar *gpu_id = g_strdup_printf("pci-dc-%s", card_obj->name_req);
                        sysobj_virt_add_simple(":/gpu/found", gpu_id, card_obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
                        g_free(gpu_id);
                    }
                }
            }
            sysobj_free(card_obj);
        }
    }
    sysobj_free(drm_obj);
}

/*  Look for this kind of thing:
 *     * /soc/gpu
 *     * /gpu@ff300000
 *
 *  Usually a gpu dt node will have ./name = "gpu"
 */
static gboolean _dt_item_callback(const sysobj *obj, gpointer user_data, gconstpointer stats) {
    if ( !g_str_has_prefix(obj->name, "gpu") )
        return SYSOBJ_FOREACH_CONTINUE;

    /* should either be NULL or @ */
    if (*(obj->name+3) == '\0' || *(obj->name+3) == '@') {
        gchar *gpu_id = g_strdup_printf("dt-%s", obj->name_req);
        sysobj_virt_add_simple(":/gpu/found", gpu_id, obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
        g_free(gpu_id);
    }
    return SYSOBJ_FOREACH_CONTINUE;
}

static void find_dt_gpu_devices() {
    sysobj_foreach_from("/sys/firmware/devicetree/base", NULL, (f_sysobj_foreach)_dt_item_callback, NULL, SO_FOREACH_NORMAL);
}

static void create_gpus() {

}

static gchar *gpu_dir(const gchar *path) {
    if (!path) return NULL; //TODO
    return g_strdup("found");
}

static sysobj_virt vol[] = {
    { .path = ":/gpu", .f_get_data = gpu_dir,
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
    { .path = ":/gpu/found", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
};

void gen_gpu() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);

    find_drm_cards();
    find_pci_vga_devices();
    find_dt_gpu_devices();
    // TODO: usb3 gpus?
}

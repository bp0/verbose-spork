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

/* listing is sorted, num is priority */
#define PFX_DRM "1-drm-"
#define PFX_PCI "2-pci-dc-"
#define PFX_DT  "3-dt-"

static void find_drm_cards() {
    sysobj *drm_obj = sysobj_new_from_fn("/sys/class/drm", NULL);
    if (drm_obj->exists) {
        sysobj_read(drm_obj, FALSE);
        for(GSList *l = drm_obj->data.childs; l; l = l->next) {
            sysobj *card_obj = sysobj_new_from_fn(drm_obj->path, (gchar*)l->data);
            if (verify_lblnum(card_obj, "card") ) {
                gchar *gpu_id = g_strdup_printf(PFX_DRM "%s", card_obj->name_req);
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
                        gchar *gpu_id = g_strdup_printf(PFX_PCI "%s", card_obj->name_req);
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
        gchar *gpu_id = g_strdup_printf(PFX_DT "%s", obj->name_req);
        sysobj_virt_add_simple(":/gpu/found", gpu_id, obj->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
        g_free(gpu_id);
    }
    return SYSOBJ_FOREACH_CONTINUE;
}

static void find_dt_gpu_devices() {
    sysobj_foreach_from("/sys/firmware/devicetree/base", NULL, (f_sysobj_foreach)_dt_item_callback, NULL, SO_FOREACH_NORMAL);
}

static int gpu_next = 0;

/* TODO: from /proc/driver/nvidia/gpus/%s/information, where %s is the pci addy
 * maybe it should just be added to the tree via kv? */
typedef struct nvgpu {
    char *model;
    char *bios_version;
    char *uuid;
} nvgpu;

#define nvgpu_new() g_new0(nvgpu, 1)
void nvgpu_free(nvgpu *s) {
    if (s) {
        g_free(s->model);
        g_free(s->bios_version);
        g_free(s->uuid);
        g_free(s);
    }
}

typedef struct {
    int type;
    gchar *name; /* gpu<gpu_next> */
    gchar *drm_card;  /* cardN */
    gchar *pci_addy;
    gchar *dt_name;
    gchar *unk;       /* link to :/gpu/found item that is unknown */

    gchar *nice_name;
    gchar *vendor_str;
    gchar *device_str;
    gchar *dt_compat; /* first item only */

    nvgpu *nv_info;
    /* ... */
} gpud;

#define gpud_new() (g_new0(gpud, 1))
static void gpud_free(gpud *s) {
    if (s) {
        g_free(s->name);
        g_free(s->drm_card);
        g_free(s->pci_addy);
        g_free(s->dt_name);
        g_free(s->unk);

        g_free(s->nice_name);
        g_free(s->vendor_str);
        g_free(s->device_str);
        g_free(s->dt_compat);
        g_free(s);
    }
}

GSList *gpu_list = NULL;
#define gpu_list_free() g_slist_free_full(gpu_list, (GDestroyNotify)gpud_free)

/* TODO: In the future, when there is more vendor specific information available in
 * the gpu struct, then more precise names can be given to each gpu */
static void make_nice_name(gpud *s) {
    /* NV information available */
    if (s->nv_info && s->nv_info->model) {
        s->nice_name = g_strdup_printf("%s %s", "NVIDIA", s->nv_info->model);
        return;
    }

    static const gchar unk_v[] = "Unknown"; /* do not...    */
    static const gchar unk_d[] = "Device";  /* ...translate */
    const gchar *vendor_str = s->vendor_str;
    const gchar *device_str = s->device_str;

    /* try and a get a "short name" for the vendor */
    if (vendor_str)
        vendor_str = vendor_get_shortest_name(vendor_str);

    if (s->dt_compat) {
        if (!vendor_str && !device_str) {
            s->nice_name = g_strdup(s->dt_compat);
            char *comma = strchr(s->nice_name, ',');
            if (comma) *comma = ' ';  /* "brcm,bcm2835-vc4" -> "brcm bcm2835-vc4" */
            return;
        }
        if (!device_str) {
            char *comma = strchr(s->dt_compat, ',');
            if (comma)
                device_str = g_strdup(comma + 1);
            else {
                s->nice_name = g_strdup_printf("%s %s", vendor_str, device_str);
                return;
            }
        }
        if (!vendor_str) {
            s->nice_name = g_strdup_printf("%s", device_str);
            return;
        }
    }

    if (!vendor_str) vendor_str = unk_v;
    if (!device_str) device_str = unk_d;

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
        gchar *full_name = strdup(device_str);
        /* Try and shorten it to the chip code name only, at least */
        char *b = strchr(full_name, '[');
        if (b) *b = 0;
        s->nice_name = g_strdup_printf("%s %s", "AMD/ATI", g_strstrip(full_name));
        g_free(full_name);
        return;
    }

    /* nothing nicer */

nice_is_over:
    s->nice_name = g_strdup_printf("%s %s", vendor_str, device_str);
}

static void gpu_scan() {
    /* look through all found and if something new is there, create a gpud */
    sysobj *flobj = sysobj_new_fast(":/gpu/found");
    GSList *found = sysobj_children(flobj, NULL, NULL, TRUE);
    for(GSList *f = found; f; f = f->next) {
        gchar *req = util_build_fn(flobj->path, (gchar*)f->data);
        sysobj *fobj = sysobj_new_fast(req);
        gchar *found_name = fobj->name_req;
        gchar *id = NULL;
        int type = 0;

        #define CHKPFX(pfx) if (!id && g_str_has_prefix(found_name, pfx)) { id = found_name + strlen(pfx); }
        CHKPFX(PFX_DRM) if (!type && id) type = 1;
        CHKPFX(PFX_PCI) if (!type && id) type = 2;
        CHKPFX(PFX_DT ) if (!type && id) type = 3;

        gpud *owner = NULL;
        for(GSList *l = gpu_list; l; l = l->next) {
            /* find owner */
            gpud *g = (gpud*)l->data;
            switch(type) {
                case 1: if (SEQ(g->drm_card, id) ) owner = g;
                    break;
                case 2: if (SEQ(g->pci_addy, id) ) owner = g;
                    break;
                case 3: if (SEQ(g->dt_name, id) ) owner = g;
                    break;
                case 0:
                default:
                    if (SEQ(g->unk, req) ) owner = g;
                    break;
            }
        } /* each gpud */

        if (!owner) {
            gpud *g = gpud_new();
            g->name = g_strdup_printf("gpu%d", gpu_next++);
            switch(type) {
                case 1:
                    g->drm_card = g_strdup(id);
                    sysobj *dev = sysobj_new_from_fn(fobj->path, "device");
                    sysobj *subsys = sysobj_new_from_fn(fobj->path, "device/subsystem");
                    sysobj *of_node = sysobj_new_from_fn(fobj->path, "device/of_node");
                    if (SEQ(subsys->name, "pci") )
                        g->pci_addy = g_strdup(dev->name);
                    if (of_node->exists) {
                        g->dt_name = g_strdup(of_node->name);
                        g->dt_compat = sysobj_raw_from_fn(of_node->path, "compatible");
                        util_strstrip_double_quotes_dumb(g->dt_compat);
                    }
                    sysobj_free(dev);
                    sysobj_free(subsys);
                    sysobj_free(of_node);
                    break;
                case 2:
                    g->pci_addy = g_strdup(id);
                    break;
                case 3:
                    g->dt_name = g_strdup(id);
                    g->dt_compat = sysobj_raw_from_fn(fobj->path, "compatible");
                    util_strstrip_double_quotes_dumb(g->dt_compat);
                    break;
                case 0:
                default:
                    g->unk = g_strdup(req);
                    break;
            }
            gpu_list = g_slist_append(gpu_list, g);
            gchar *gpu_path = util_build_fn(":/gpu", g->name);
            sysobj_virt_add_simple(gpu_path, NULL, "*", VSO_TYPE_DIR);
            if (g->drm_card) {
                gchar *lt = g_strdup_printf(":/gpu/found/" PFX_DRM "%s", g->drm_card);
                sysobj_virt_add_simple(gpu_path, g->drm_card, lt, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
                g_free(lt);
            }
            if (g->pci_addy) {
                gchar *lt = g_strdup_printf(":/gpu/found/" PFX_PCI "%s", g->pci_addy);
                sysobj_virt_add_simple(gpu_path, g->pci_addy, lt, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
                /* name by pci.ids */
                gchar *v_str = sysobj_raw_from_printf("%s/%s", lt, "vendor");
                gchar *d_str = sysobj_raw_from_printf("%s/%s", lt, "device");
                uint32_t v = strtol(v_str, NULL, 16);
                uint32_t d = strtol(d_str, NULL, 16);
                g->vendor_str = sysobj_raw_from_printf(":/pci/pci.ids/%04x/name", v);
                g->device_str = sysobj_raw_from_printf(":/pci/pci.ids/%04x/%04x/name", v, d);
                g_free(v_str);
                g_free(d_str);
                g_free(lt);
            }
            if (g->dt_name) {
                gchar *lt = g_strdup_printf(":/gpu/found/" PFX_DT "%s", g->dt_name);
                sysobj_virt_add_simple(gpu_path, g->dt_name, lt, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
                g_free(lt);
                /* name by dt.ids */
                g->vendor_str = sysobj_raw_from_printf(":/devicetree/dt.ids/%s/vendor", g->dt_compat);
                g->device_str = sysobj_raw_from_printf(":/devicetree/dt.ids/%s/name", g->dt_compat);
            }
            make_nice_name(g);
            sysobj_virt_add_simple(gpu_path, "name", g->nice_name, VSO_TYPE_STRING );
            g_free(gpu_path);
        }
        sysobj_free(fobj);
        g_free(req);
    } /* each found */
    sysobj_free(flobj);
}

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

static gchar *gpu_data(const gchar *path) {
    gchar name[16] = "";

    if (!path) {
        /* cleanup */
        gpu_list_free();
        return NULL;
    }

    if (SEQ(path, ":/gpu") ) {
        /* dir */
        gpu_scan();

        /* use manual dir creation instead of
         * auto ("*") so that "found" and
         * "list" may be hidden from listing */
        #define HIDE_STUFF 1
        gchar *ret = HIDE_STUFF ? NULL : g_strdup("found\n" "list\n");
        for(GSList *l = gpu_list; l; l = l->next) {
            gpud *g = (gpud*)l->data;
            ret = appfs(ret, "\n", "%s", g->name);
        }
        return ret;
    }

    return NULL;
}

static gchar *gpu_summary(const gchar *path) {
    if (!path) return NULL; /* no cleanup */

    gchar *ret = NULL;
    for(GSList *l = gpu_list; l; l = l->next) {
        gpud *g = (gpud*)l->data;
        ret = appfs(ret, " + ", "%s", g->nice_name);
    }
    return ret;
}

static sysobj_virt vol[] = {
    { .path = ":/gpu", .f_get_data = gpu_data, /* note: manual dir creation */
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST },
    { .path = ":/gpu/list", .f_get_data = gpu_summary,
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST },
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

    gpu_scan(); /* initial */
}

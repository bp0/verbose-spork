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
#include "sysobj_extras.h"
#include "util_pci.h"
#include "vendor.h"

/* virt get funcs */
static gchar *gpu_data(const gchar *path);
static gchar *gpu_driver(const gchar *path);
static gchar *gpu_core_khz(const gchar *path);

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
            if (verify_pci_addy(card_obj->name) ) {
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
    if (obj->data.is_dir && *(obj->name+3) == '\0' || *(obj->name+3) == '@') {
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

typedef struct {
    int type;
    gchar *name; /* gpu<gpu_next> */
    gchar *drm_card;  /* cardN */
    gchar *pci_addy;
    gchar *dt_name;
    gchar *unk;       /* link to :/gpu/found item that is unknown */

    gchar *device_path;
    gchar *dt_path;
    gchar *nice_name;
    gchar *vendor_str;
    gchar *device_str;
    gchar *dt_compat; /* first item only */

    const Vendor *vendor;
    gboolean nv_info; /* nvidia info was found in /proc/driver/nvidia/gpus */
} gpud;

#define gpud_new() (g_new0(gpud, 1))
static void gpud_free(gpud *s) {
    if (s) {
        g_free(s->name);
        g_free(s->drm_card);
        g_free(s->pci_addy);
        g_free(s->dt_name);
        g_free(s->device_path);
        g_free(s->unk);

        g_free(s->dt_path);
        g_free(s->nice_name);
        g_free(s->vendor_str);
        g_free(s->device_str);
        g_free(s->dt_compat);

        g_free(s);
    }
}

static GSList *gpu_list = NULL;
#define gpu_list_free() g_slist_free_full(gpu_list, (GDestroyNotify)gpud_free)

static GMutex gpu_list_lock;

static void gpu_pci_hwmon(gpud *g) {
    /* for hwmon_attr_decode_name */
    gchar *type = NULL, *attrib = NULL;
    int index;
    gboolean is_value;

    if (!g->name || !g->pci_addy) return;

    gchar *gpu_path = util_build_fn(":/gpu", g->name);

    /* link hwmon inputs */
    sysobj *pcid = sysobj_new_from_printf("/sys/bus/pci/devices/%s", g->pci_addy);
    sysobj *hwmon_list = sysobj_new_from_fn(pcid->path, "hwmon");
    if (pcid->exists && hwmon_list->exists) {
        sysobj_read(hwmon_list, FALSE);
        for(GSList *l = hwmon_list->data.childs; l; l = l->next) {
            sysobj *hwmon = sysobj_new_from_fn(hwmon_list->path, (gchar*)l->data);
            sysobj_read(hwmon, FALSE);
            gchar *hwmon_name = g_strchomp(sysobj_raw_from_fn(hwmon->path, "name") );
            if (!hwmon_name)
                hwmon_name = g_strdup(hwmon->name);
            for(GSList *m = hwmon->data.childs; m; m = m->next) {
                gchar *sensor = (gchar*)m->data;
                /* hwmon_attr_decode_name() will free the strings before setting them, if
                 * they are not null, and reset is_value */
                if (hwmon_attr_decode_name(sensor, &type, &index, &attrib, &is_value) )
                if (is_value) {
                    gchar *sl = NULL, *st = NULL, *lbl = NULL;
                    lbl = sysobj_raw_from_fn(hwmon->path,
                        auto_free(hwmon_attr_encode_name(type, index, "label")));
                    if (lbl) {
                        g_strchomp(lbl);
                        sl = g_strdup_printf("%s.%s", hwmon_name, lbl);
                    } else
                        sl = g_strdup_printf("%s.%s", hwmon_name,
                            (gchar*)auto_free(hwmon_attr_encode_name(type, index, NULL)) );
                    st = util_build_fn(hwmon->path, sensor);

                    sysobj_virt_add_simple(gpu_path, sl, st, VSO_TYPE_SYMLINK );
                    g_free(sl);
                    g_free(st);
                    g_free(lbl);
                }
            }
            sysobj_free(hwmon);
            g_free(hwmon_name);
        }
    }
    sysobj_free(pcid);
    sysobj_free(hwmon_list);
    g_free(gpu_path);
}

static void gpu_nv(gpud *g) {
    if (!g->name || !g->pci_addy) return;
    gchar *gpu_path = util_build_fn(":/gpu", g->name);
    sysobj *nv = sysobj_new_from_printf("/proc/driver/nvidia/gpus/%s/information", g->pci_addy);
    if (nv->exists) {
        g->nv_info = TRUE;
        gchar *nv_path = util_build_fn(gpu_path, "nvidia");
        sysobj_virt_add_simple(nv_path, NULL, "*", VSO_TYPE_DIR);
        sysobj_read(nv, FALSE);
        sysobj_virt_from_lines(nv_path, nv->data.str, TRUE);
        g_free(nv_path);
    }
    sysobj_free(nv);
    g_free(gpu_path);
}

static void gpu_intel(gpud *g) {
    if (!g->name || !g->drm_card) return;
    gchar *gpu_path = util_build_fn(":/gpu", g->name);

    sysobj *min_freq = sysobj_new_from_printf("/sys/class/drm/%s/%s", g->drm_card, "gt_min_freq_mhz");
    sysobj *max_freq = sysobj_new_from_printf("/sys/class/drm/%s/%s", g->drm_card, "gt_max_freq_mhz");
    sysobj *act_freq = sysobj_new_from_printf("/sys/class/drm/%s/%s", g->drm_card, "gt_act_freq_mhz");
    if (min_freq->exists)
        sysobj_virt_add_simple_mkpath(gpu_path, "intel/min_freq", min_freq->path, VSO_TYPE_SYMLINK );
    if (max_freq->exists)
        sysobj_virt_add_simple_mkpath(gpu_path, "intel/max_freq", max_freq->path, VSO_TYPE_SYMLINK );
    if (act_freq->exists)
        sysobj_virt_add_simple_mkpath(gpu_path, "intel/actual_freq", act_freq->path, VSO_TYPE_SYMLINK );
    sysobj_free(min_freq);
    sysobj_free(max_freq);
    sysobj_free(act_freq);
    g_free(gpu_path);
}

static void gpu_pci_var(gpud *g) {
    if (!g->name || !g->pci_addy) return;

    static struct { const char *target, *name; } find_items[] = {
        { "vbios_version", "vbios_version" },
        { "boot_vga", "boot_vga" },
        { "max_link_speed", "pcie.max_link_speed" },
        { "current_link_speed", "pcie.current_link_speed" },
        { "max_link_width", "pcie.max_link_width" },
        { "current_link_width", "pcie.current_link_width" },
        { "pp_dpm_mclk", "amd/pp_dpm_mclk" },
        { "pp_dpm_sclk", "amd/pp_dpm_sclk" },
    };

    gchar *gpu_path = util_build_fn(":/gpu", g->name);
    for(int i = 0; i < (int)G_N_ELEMENTS(find_items); i++) {
        sysobj *obj = sysobj_new_from_fn(g->device_path, find_items[i].target);
        if (obj->exists)
            sysobj_virt_add_simple_mkpath(gpu_path, find_items[i].name, obj->path, VSO_TYPE_SYMLINK );
        sysobj_free(obj);
    }
    g_free(gpu_path);
}

static void gpu_dt_opp(gpud *g) {
    if (!g->name || !g->dt_name) return;

    gchar *gpu_opp_path = g_strdup_printf(":/gpu/%s/opp", g->name);
    gchar *oppkv = dtr_get_opp_kv(g->dt_path, "");

    if (oppkv) {
        sysobj_virt_add_simple(gpu_opp_path, NULL, "*", VSO_TYPE_DIR);
        sysobj_virt_from_kv(gpu_opp_path, oppkv);

        gchar *ver = sysobj_raw_from_fn(gpu_opp_path, "version");
        if (SEQ(ver, "2") ) {
            /* link to of_node */
            gchar *ref = sysobj_raw_from_fn(gpu_opp_path, "ref");
            gchar *target = util_build_fn("/sys/firmware/devicetree/base", ref);
            sysobj_virt_add_simple(gpu_opp_path, "of_node", target, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN);
            g_free(ref);
        }
        g_free(ver);
    }

    g_free(oppkv);
    g_free(gpu_opp_path);
}

static gboolean _dev_by_of_node_callback(const sysobj *obj, gpud *g, gconstpointer stats) {
    if (SEQ(obj->name_req, "of_node")
        && SEQ(obj->path, g->dt_path) ) {
        g->device_path = sysobj_parent_path_ex(obj, TRUE);
        return SYSOBJ_FOREACH_STOP;
    }
    return SYSOBJ_FOREACH_CONTINUE;
}

static void find_dev_by_of_node(gpud *g) {
    sysobj_foreach_from("/sys/devices/platform", NULL, (f_sysobj_foreach)_dev_by_of_node_callback, g, SO_FOREACH_NORMAL);
}

/* export */
void sysobj_virt_add_vendor_match(gchar *base, gchar *name, const Vendor *vendor) {
    if (!vendor) return;
    gchar *path = util_build_fn(base, name);
    sysobj_virt_add_simple_mkpath(path, "name", vendor->name, VSO_TYPE_STRING);
    if (vendor->name_short && strlen(vendor->name_short) )
        sysobj_virt_add_simple(path, "name_short", vendor->name_short, VSO_TYPE_STRING);
    if (vendor->url && strlen(vendor->url) )
        sysobj_virt_add_simple(path, "url", vendor->url, VSO_TYPE_STRING);
    if (vendor->url_support && strlen(vendor->url_support) )
        sysobj_virt_add_simple(path, "url_support", vendor->url_support, VSO_TYPE_STRING);
    if (vendor->ansi_color && strlen(vendor->ansi_color) )
        sysobj_virt_add_simple(path, "ansi_color", vendor->ansi_color, VSO_TYPE_STRING);
}

static void gpu_scan() {
    g_mutex_lock(&gpu_list_lock);
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
                    g->device_path = g_strdup(dev->path);
                    if (SEQ(subsys->name, "pci") )
                        g->pci_addy = g_strdup(dev->name);
                    if (of_node->exists) {
                        g->dt_name = g_strdup(of_node->name);
                        g->dt_path = g_strdup(of_node->path);
                        g->dt_compat = sysobj_raw_from_fn(of_node->path, "compatible");
                        util_strstrip_double_quotes_dumb(g->dt_compat);
                    }
                    sysobj_free(dev);
                    sysobj_free(subsys);
                    sysobj_free(of_node);
                    break;
                case 2:
                    g->pci_addy = g_strdup(id);
                    g->device_path = g_strdup(fobj->path);
                    break;
                case 3:
                    g->dt_name = g_strdup(id);
                    g->dt_path = g_strdup(fobj->path);
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
                sysobj_virt_add_simple(gpu_path, "drm", lt, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
                g_free(lt);
                /* extra stuff */
                gpu_intel(g);
            }
            if (g->pci_addy) {
                gchar *lt = g_strdup_printf(":/gpu/found/" PFX_PCI "%s", g->pci_addy);
                sysobj_virt_add_simple(gpu_path, "pci", lt, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
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
                /* extra stuff */
                gpu_pci_hwmon(g);
                gpu_pci_var(g);
                gpu_nv(g);
            }
            if (g->dt_name) {
                /* device path in platform */
                if (!g->device_path)
                    find_dev_by_of_node(g);
                /* still no device? link the dt node */
                if (!g->device_path) {
                    gchar *lt = g_strdup_printf(":/gpu/found/" PFX_DT "%s", g->dt_name);
                    sysobj_virt_add_simple(gpu_path, "of_node", lt, VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
                    g_free(lt);
                }
                /* name by dt.ids */
                g->vendor_str = sysobj_raw_from_printf(":/devicetree/dt.ids/%s/vendor", g->dt_compat);
                g->device_str = sysobj_raw_from_printf(":/devicetree/dt.ids/%s/name", g->dt_compat);
                /* extra stuff */
                gpu_dt_opp(g);
            }

            /* device name strings */
            if (g->vendor_str) {
                sysobj_virt_add_simple_mkpath(gpu_path, "name/vendor_name", g->vendor_str, VSO_TYPE_STRING );
                g->vendor = vendor_match(g->vendor_str, NULL);
                if (g->vendor)
                    sysobj_virt_add_vendor_match(gpu_path, "name/vendor", g->vendor);
            }
            if (g->device_str)
                sysobj_virt_add_simple_mkpath(gpu_path, "name/device_name", g->device_str, VSO_TYPE_STRING );

            if (g->device_path) {
                sysobj *dev = sysobj_new_fast(g->device_path);
                if (!g->drm_card && !g->pci_addy)
                    sysobj_virt_add_simple(gpu_path, "device", dev->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );

                /* devfreq */
                gchar *devfreq_path = g_strdup_printf("%s/devfreq/%s", dev->path, dev->name);
                sysobj *devfreq = sysobj_new_fast(devfreq_path);
                if (devfreq->exists)
                    sysobj_virt_add_simple(gpu_path, "devfreq", devfreq->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
                sysobj_free(devfreq);
                g_free(devfreq_path);

                sysobj_free(dev);
            }

            sysobj_virt *vo = NULL;

            /* driver finder */
            vo = sysobj_virt_new();
            vo->path = util_build_fn(gpu_path, "driver_kmod");
            vo->type = VSO_TYPE_STRING;
            vo->f_get_data = gpu_driver;
            sysobj_virt_add(vo);

            /* clock freq finders */
            vo = sysobj_virt_new();
            vo->path = util_build_fn(gpu_path, "core_khz_max");
            vo->type = VSO_TYPE_STRING;
            vo->f_get_data = gpu_core_khz;
            sysobj_virt_add(vo);
            vo = sysobj_virt_new();
            vo->path = util_build_fn(gpu_path, "core_khz_min");
            vo->type = VSO_TYPE_STRING;
            vo->f_get_data = gpu_core_khz;
            sysobj_virt_add(vo);
            vo = sysobj_virt_new();
            vo->path = util_build_fn(gpu_path, "core_khz_cur");
            vo->type = VSO_TYPE_STRING;
            vo->f_get_data = gpu_core_khz;
            sysobj_virt_add(vo);

            g_free(gpu_path);
        }
        sysobj_free(fobj);
        g_free(req);
    } /* each found */
    sysobj_free(flobj);
    g_mutex_unlock(&gpu_list_lock);
}

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

/* find the driver module */
static gchar *gpu_driver(const gchar *path) {
    if (!path) return NULL;
    gchar *ret = NULL;
    sysobj *mod = sysobj_new_from_fn(path, "../drm/device/driver/module");
    if (!mod->exists) {
        sysobj_free(mod);
        mod = sysobj_new_from_fn(path, "../pci/driver/module");
    }
    if (mod->exists) {
        gchar *ver = sysobj_raw_from_fn(mod->path, "version");
        ret = ver
            ? g_strdup_printf("%s/%s", mod->name, ver)
            : g_strdup(mod->name);
        g_free(ver);
    }
    sysobj_free(mod);
    return ret;
}

static gchar *gpu_core_khz(const gchar *path) {
    if (!path) return NULL;
    gchar *ret = NULL;

    int t = 0; /* _cur */
    if (g_str_has_suffix(path, "_min") ) t = 1;
    if (g_str_has_suffix(path, "_max") ) t = 2;

    /* SOURCE: MIN, MAX, CUR
     * intel(MHZ): intel/min_freq, intel/max_freq, intel/actual_freq
     * amdgpu(MHZ): amd/pp_dpm_sclk-> first line, last line, line with *
     * devfreq(HZ): devfreq/available_frequencies (first, last), devfreq/cur_freq
     * opp: opp/khz_min, opp/khz_max, NA
     */

    sysobj *tobj = NULL;

    switch(t) {
        case 0: /* cur */
            tobj = sysobj_new_fast_from_fn(path, "../intel/actual_freq");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                double v = strtoll(tobj->data.str, NULL, 10);
                ret = g_strdup_printf("%lf", v * 1000); /* src in mhz */
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../amd/pp_dpm_sclk");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                gchar *b = strchr(tobj->data.str, '*');
                if (b) {
                    *b = 0; b = strrchr(tobj->data.str, ':');
                    if (b) {
                        double v = strtoll(b+2, NULL, 10);
                        ret = g_strdup_printf("%lf", v * 1000); /* src in mhz */
                    }
                }
                sysobj_free(tobj);
                if (ret) break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../devfreq/cur_freq");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                double v = strtoll(tobj->data.str, NULL, 10);
                ret = g_strdup_printf("%lf", v / 1000); /* src in hz */
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            /* opp doesn't offer cur, but if opp then devfreq should have been
             * available anyway */
            break;

        case 1: /* min */
            tobj = sysobj_new_fast_from_fn(path, "../intel/min_freq");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                double v = strtoll(tobj->data.str, NULL, 10);
                ret = g_strdup_printf("%lf", v * 1000); /* src in mhz */
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../amd/pp_dpm_sclk");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                gchar *b = strchr(tobj->data.str, ':');
                if (b) {
                    double v = strtoll(b+2, NULL, 10);
                    ret = g_strdup_printf("%lf", v * 1000); /* src in mhz */
                }
                sysobj_free(tobj);
                if (ret) break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../devfreq/available_frequencies");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                gchar *b = strrchr(tobj->data.str, ' ');
                double v = strtoll(b ? b : tobj->data.str, NULL, 10);
                ret = g_strdup_printf("%lf", v / 1000); /* src in hz */
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../opp/khz_min");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                ret = g_strdup(tobj->data.str);
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            break;

        case 2: /* max */
            tobj = sysobj_new_fast_from_fn(path, "../intel/max_freq");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                double v = strtoll(tobj->data.str, NULL, 10);
                ret = g_strdup_printf("%lf", v * 1000); /* src in mhz */
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../amd/pp_dpm_sclk");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                gchar *b = strrchr(tobj->data.str, ':');
                if (b) {
                    double v = strtoll(b+2, NULL, 10);
                    ret = g_strdup_printf("%lf", v * 1000); /* src in mhz */
                }
                sysobj_free(tobj);
                if (ret) break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../devfreq/available_frequencies");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                double v = strtoll(tobj->data.str, NULL, 10);
                ret = g_strdup_printf("%lf", v / 1000); /* src in hz */
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            tobj = sysobj_new_fast_from_fn(path, "../opp/khz_max");
            if (tobj->exists) {
                sysobj_read(tobj, FALSE);
                ret = g_strdup(tobj->data.str);
                sysobj_free(tobj);
                break;
            } else sysobj_free(tobj);
            break;
    }
    return ret;
}

static gchar *gpu_data(const gchar *path) {
    if (!path) {
        /* cleanup */
        gpu_list_free();
        return NULL;
    }

    if (SEQ(path, ":/gpu") ) {
        /* dir */
        gpu_scan();

        /* use manual dir creation instead of
         * auto ("*" or NULL) so that "found"
         * may be hidden from listing */
        #define HIDE_STUFF 1
        gchar *ret = HIDE_STUFF ? g_strdup("") : g_strdup("found\n" "list\n");
        for(GSList *l = gpu_list; l; l = l->next) {
            gpud *g = (gpud*)l->data;
            ret = appfs(ret, "\n", "%s", g->name);
        }
        return ret;
    }

    return NULL;
}

static sysobj_virt vol[] = {
    { .path = ":/gpu", .f_get_data = gpu_data, /* note: manual dir creation */
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST | VSO_TYPE_CLEANUP },
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

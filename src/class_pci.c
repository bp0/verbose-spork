
#include "sysobj.h"
#include "util_pci.h"

#define SYSFS_PCI "/sys/bus/pci"

gchar* pci_log = NULL;
GSList *pci_id_list = NULL;

static gchar *pci_format(sysobj *obj, int fmt_opts);
static guint pci_flags(sysobj *obj);
static gchar *pci_messages(const gchar *path);
static double pci_update_interval(sysobj *obj);
static gboolean pci_verify_idcomp(sysobj *obj);
static gchar *pci_format_idcomp(sysobj *obj, int fmt_opts);
static gchar *pci_format_device(sysobj *obj, int fmt_opts);

void class_pci_cleanup();

#define CLS_PCI_FLAGS OF_GLOB_PATTERN | OF_CONST

static sysobj_class cls_pci[] = {
  { .tag = "pci/device_id", .pattern = "/sys/devices*/????:??:??.?/*", .flags = CLS_PCI_FLAGS,
    .f_verify = pci_verify_idcomp,
    .f_format = pci_format_idcomp, .f_flags = pci_flags, .f_update_interval = pci_update_interval },
  { .tag = "pci/device", .pattern = "/sys/devices*/????:??:??.?", .flags = CLS_PCI_FLAGS,
    .f_format = pci_format_device, .f_flags = pci_flags, .f_update_interval = pci_update_interval },
  /* all under :pci */
  { .tag = "pci", .pattern = ":pci/*", .flags = CLS_PCI_FLAGS,
    .f_format = pci_format, .f_flags = pci_flags, .f_update_interval = pci_update_interval, .f_cleanup = class_pci_cleanup },
};

static sysobj_virt vol[] = {
    { .path = ":pci/bus", .str = SYSFS_PCI,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":pci/_messages", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = pci_messages, .f_get_type = NULL },
    { .path = ":pci", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

void pci_msg(char *fmt, ...) {
    gchar *buf, *tmp;
    va_list args;

    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);

    tmp = g_strdup_printf("%s%s\n", pci_log, buf);
    g_free(pci_log);
    pci_log = tmp;
}

static gchar *pci_messages(const gchar *path) {
    PARAM_NOT_UNUSED(path);
    return g_strdup(pci_log ? pci_log : "");
}

static gchar *pci_format(sysobj *obj, int fmt_opts) {
    return simple_format(obj, fmt_opts);
}

static guint pci_flags(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return CLS_PCI_FLAGS;
}

static double pci_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 10.0;
}

static util_pci_id *pci_find_id(gchar *address) {
    GSList *l = pci_id_list;
    while(l) {
        util_pci_id *d = (util_pci_id*)l->data;
        if (!g_strcmp0(address, d->address))
            return d;
        l = l->next;
    }
    return NULL;
}

static const gchar *pci_idcomps[] =
{ "vendor", "device", "subsystem_vendor", "subsystem_device", "class" };

static gboolean pci_verify_idcomp(sysobj *obj) {
    int i = 0;
    for (i = 0; i < G_N_ELEMENTS(pci_idcomps); i++ ) {
        if (!g_strcmp0(obj->name, pci_idcomps[i]) )
            return TRUE;
    }
    return FALSE;
}

static util_pci_id dev_not_found = { 0 };

static gchar *pci_format_idcomp(sysobj *obj, int fmt_opts) {
    gchar *pn = sysobj_parent_name(obj);
    util_pci_id *d = pci_find_id(pn);
    g_free(pn);
    if (!d) d = &dev_not_found;
    gchar *value_str = NULL;
    if (!g_strcmp0(obj->name, "vendor") )
        value_str = d->vendor_str ? d->vendor_str : "Unknown";
    else if (!g_strcmp0(obj->name, "device") )
        value_str = d->device_str ? d->device_str : "Device";
    else if (!g_strcmp0(obj->name, "subsystem_vendor") )
        value_str = d->sub_vendor_str ? d->sub_vendor_str : "Unknown";
    else if (!g_strcmp0(obj->name, "subsystem_device") )
        value_str = d->sub_device_str ? d->sub_device_str : "Device";
    else if (!g_strcmp0(obj->name, "class") )
        value_str = d->dev_class_str ? d->dev_class_str : "Unknown";
    if (value_str) {
        uint32_t value = strtol(obj->data.str, NULL, 16);
        return g_strdup_printf("[%04x] %s", value, value_str);
    }
    return simple_format(obj, fmt_opts);
}

static gchar *pci_format_device(sysobj *obj, int fmt_opts) {
    util_pci_id *d = pci_find_id(obj->name);
    if (d) {
        return g_strdup_printf("%s %s",
            d->vendor_str ? d->vendor_str : "Unknown",
            d->device_str ? d->device_str : "Device");
    }
    return simple_format(obj, fmt_opts);
}

void class_pci_cleanup() {
    if (pci_id_list) {
        g_slist_free_full(pci_id_list, (GDestroyNotify)util_pci_id_free);
    }
}

void pci_scan() {
    if (pci_id_list) return; /* already done */

    sysobj *obj = sysobj_new_from_fn(SYSFS_PCI, "devices");

    if (!obj->exists) {
        pci_msg("pci device list not found at %s/devices", SYSFS_PCI);
        sysobj_free(obj);
        return;
    }
    GSList *devs = sysobj_children(obj, NULL, NULL, TRUE);
    GSList *l = devs;
    while(l) {
        util_pci_id *pid = g_new0(util_pci_id, 1);
        gchar *dev = (gchar*)l->data;
        gchar *dev_path = g_strdup_printf("%s/%s", obj->path, dev);
        pid->address = g_strdup(dev);
        pid->vendor = sysobj_uint32_from_fn(dev_path, "vendor", 16);
        pid->device = sysobj_uint32_from_fn(dev_path, "device", 16);
        pid->sub_vendor = sysobj_uint32_from_fn(dev_path, "subsystem_vendor", 16);
        pid->sub_device = sysobj_uint32_from_fn(dev_path, "subsystem_device", 16);
        pid->dev_class = sysobj_uint32_from_fn(dev_path, "class", 16);
        pci_id_list = g_slist_append(pci_id_list, pid);
        g_free(dev_path);
        l = l->next;
    }
    if (pci_id_list)
        util_pci_ids_lookup_list(pci_id_list);

    sysobj_free(obj);
}

void class_pci() {
    int i = 0;

    pci_log = g_strdup("");

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_pci); i++) {
        class_add(&cls_pci[i]);
    }

    //TODO: should wait until first access to setup
    //other classes may not be ready
    pci_scan();
}

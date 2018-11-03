
#include "sysobj.h"

static sysobj_class *cls_dmi_id, *cls_dmidecode;

static const struct { gchar *rp; gchar *lbl; int extra_flags; } dmi_id_items[] = {
    { "id",  N_("Desktop Management Interface (DMI) product information"), OF_NONE },
    { "bios_vendor",       N_("BIOS Vendor"), OF_IS_VENDOR },
    { "bios_version",      N_("BIOS Version"), OF_NONE },
    { "bios_date",         N_("BIOS Date"), OF_NONE },
    { "sys_vendor",        N_("Product Vendor"), OF_IS_VENDOR },
    { "product_name",      N_("Product Name"), OF_NONE },
    { "product_version",   N_("Product Version"), OF_NONE },
    { "product_serial",    N_("Product Serial"), OF_REQ_ROOT },
    { "product_uuid",      N_("Product UUID"), OF_NONE },
    { "product_sku",       N_("Product SKU"), OF_NONE },
    { "product_family",    N_("Product Family"), OF_NONE },
    { "board_vendor",      N_("Board Vendor"), OF_IS_VENDOR },
    { "board_name",        N_("Board Name"), OF_NONE },
    { "board_version",     N_("Board Version"), OF_NONE },
    { "board_serial",      N_("Board Serial"), OF_REQ_ROOT },
    { "board_asset_tag",   N_("Board Asset Tag"), OF_NONE },
    { "chassis_vendor",    N_("Chassis Vendor"), OF_IS_VENDOR },
    { "chassis_version",   N_("Chassis Version"), OF_NONE },
    { "chassis_serial",    N_("Chassis Serial"), OF_REQ_ROOT },
    { "chassis_asset_tag", N_("Chassis Asset Tag"), OF_NONE },
    { NULL, NULL, 0 }
};

int dmi_id_lookup(const gchar *key) {
    int i = 0;
    while(dmi_id_items[i].rp) {
        if (strcmp(key, dmi_id_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

gboolean dmi_value_is_placeholder(sysobj *obj) {
    gboolean ret = FALSE;
    gchar *v = NULL, *chk = NULL, *p = NULL;
    int i = 0;

    static const char *common[] = {
        "To be filled by O.E.M.",
        "Default String",
        "System Product Name",
        "System Manufacturer",
        "System Version",
        "Rev X.0x", /* ASUS board version nonsense */
        "x.x",      /* Gigabyte board version nonsense */
        "NA",
        "SKU",
        NULL
    };

    /* only consider known items */
    i = dmi_id_lookup(obj->name);
    if (i == -1)
        return FALSE;

    v = (obj->data.was_read && obj->data.is_utf8) ? obj->data.str : NULL;
    if (v) {
        v = strdup(v);
        g_strchomp(v);
        chk = g_strdup(v);

        i = 0;
        while(common[i]) {
            if (!strcasecmp(common[i], v))
                goto dmi_ignore_yes;
            i++;
        }

        /* Zotac version nonsense */
        p = chk;
        while (*p != 0) { *p = 'x'; p++; } /* all X */
        if (!strcmp(chk, v))
            goto dmi_ignore_yes;
        p = chk;
        while (*p != 0) { *p = '0'; p++; } /* all 0 */
        if (!strcmp(chk, v))
            goto dmi_ignore_yes;

        /*... more, I'm sure. */

        goto dmi_ignore_no;
dmi_ignore_yes:
        ret = TRUE;
dmi_ignore_no:
        g_free(v);
        g_free(chk);
    }
    return ret;
}

gchar *chassis_type_str(sysobj *obj, int fmt_opts) {
    static const char *types[] = {
        N_("Invalid chassis type (0)"),
        N_("Other"),
        N_("Unknown chassis type"),
        N_("Desktop"),
        N_("Low-profile Desktop"),
        N_("Pizza Box"),
        N_("Mini Tower"),
        N_("Tower"),
        N_("Portable"),
        N_("Laptop"),
        N_("Notebook"),
        N_("Handheld"),
        N_("Docking Station"),
        N_("All-in-one"),
        N_("Subnotebook"),
        N_("Space-saving"),
        N_("Lunch Box"),
        N_("Main Server Chassis"),
        N_("Expansion Chassis"),
        N_("Sub Chassis"),
        N_("Bus Expansion Chassis"),
        N_("Peripheral Chassis"),
        N_("RAID Chassis"),
        N_("Rack Mount Chassis"),
        N_("Sealed-case PC"),
        N_("Multi-system"),
        N_("CompactPCI"),
        N_("AdvancedTCA"),
        N_("Blade"),
        N_("Blade Enclosing"),
        N_("Tablet"),
        N_("Convertible"),
        N_("Detachable"),
        N_("IoT Gateway"),
        N_("Embedded PC"),
        N_("Mini PC"),
        N_("Stick PC"),
    };

    FMT_OPTS_IGNORE();

    if (obj && obj->data.str) {
        int chassis_type = strtol(obj->data.str, NULL, 10);
        if (chassis_type >= 0 && chassis_type < (int)G_N_ELEMENTS(types))
            return g_strdup_printf("[%d] %s", chassis_type, _(types[chassis_type]));
        return g_strdup_printf("[%d]", chassis_type);
    }
    return NULL;
}

gboolean dmi_id_verify(sysobj *obj) {
/*
    int i = dmi_id_lookup(obj->name);
    if (i != -1)
        return TRUE;
*/
    if (!strcmp(obj->name, "id")) {
        if (verify_parent_name(obj, "dmi"))
            return TRUE;
    } else if (verify_parent(obj, "/dmi/id"))
        return TRUE;
    return FALSE;
}

const gchar *dmi_id_label(sysobj *obj) {
    int i = dmi_id_lookup(obj->name);
    if (i != -1)
        return _(dmi_id_items[i].lbl);
    return NULL;
}

gchar *dmi_id_format(sysobj *obj, int fmt_opts) {
    if (!strcmp(obj->name, "chassis_type"))
        return chassis_type_str(obj, fmt_opts);

    if ((fmt_opts & FMT_OPT_NO_JUNK) && dmi_value_is_placeholder(obj) ) {
        if (fmt_opts & FMT_OPT_NULL_IF_EMPTY) {
            return NULL;
        } else {
            gchar *ret = NULL, *v = g_strdup(obj->data.str);
            g_strchomp(v);
            if (fmt_opts & FMT_OPT_HTML || fmt_opts & FMT_OPT_PANGO)
                ret = g_strdup_printf("<s>%s</s>", v);
            else if (fmt_opts & FMT_OPT_ATERM)
                ret = g_strdup_printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, v);
            else
                ret = g_strdup_printf("[X] %s", v);
            g_free(v);
            return ret;
        }
    }
    return simple_format(obj, fmt_opts);
}

/* remember to handle obj == NULL */
guint dmi_id_flags(sysobj *obj) {
    if (obj) {
        int i = dmi_id_lookup(obj->name);
        if (i != -1)
            return obj->cls->flags | dmi_id_items[i].extra_flags;
    }
    return cls_dmi_id ? cls_dmi_id->flags : OF_NONE;
}

gchar sysfs_map_dir[1024] = "";
gchar string_dir[1024] = "";

static struct {
    char *id;       /* dmidecode -s ... */
    char *path;
} tab_dmi_sysfs[] = {
    /* dmidecode -> sysfs */
    { "bios-release-date", "id/bios_date" },
    { "bios-vendor", "id/bios_vendor" },
    { "bios-version", "id/bios_version" },
    { "baseboard-product-name", "id/board_name" },
    { "baseboard-manufacturer", "id/board_vendor" },
    { "baseboard-version", "id/board_version" },
    { "baseboard-serial-number", "id/board_serial" },
    { "baseboard-asset-tag", "id/board_asset_tag" },
    { "system-product-name", "id/product_name" },
    { "system-manufacturer", "id/sys_vendor" },
    { "system-serial-number", "id/product_serial" },
    { "system-product-family", "id/product_family" },
    { "system-version", "id/product_version" },
    { "system-uuid", "product_uuid" },
    { "chassis-type", "id/chassis_type" },
    { "chassis-serial-number", "id/chassis_serial" },
    { "chassis-manufacturer", "id/chassis_vendor" },
    { "chassis-version", "id/chassis_version" },
    { "chassis-asset-tag", "id/chassis_asset_tag" },
    { "processor-family", NULL },
    { "processor-manufacturer", NULL },
    { "processor-version", NULL },
    { "processor-frequency", NULL },
    { NULL, NULL }
};

gchar *dmidecode_map(const gchar *name) {
    int i = 0;
    while(tab_dmi_sysfs[i].id) {
        if (strcmp(name, tab_dmi_sysfs[i].id) == 0) {
            if (tab_dmi_sysfs[i].path)
                return g_strdup_printf("/sys/devices/virtual/dmi/%s", tab_dmi_sysfs[i].path);
            else
                break;
        }
        i++;
    }
    return NULL;
}

gchar *dmidecode_get_str(const gchar *path) {
    gchar *name = g_path_get_basename(path);
    gchar *ret = NULL;
    if (strcmp(name, "--string") == 0) {
        ret = g_strdup(string_dir);
    } else {
        ret = NULL; //TODO: get from dmidecode
    }
    g_free(name);
    return ret;
}

gchar *dmidecode_get_link(const gchar *path) {
    gchar *name = g_path_get_basename(path);
    gchar *ret = NULL;
    if (strcmp(name, "sysfs_map") == 0) {
        ret = g_strdup(sysfs_map_dir);
    } else {
        ret = dmidecode_map(name);
    }
    g_free(name);
    return ret;
}

gchar *dmidecode_get_best(const gchar *path) {
    gchar *name = g_path_get_basename(path);
    gchar *ret = NULL;
    if (strcmp(name, "best_available") == 0) {
        ret = g_strdup(string_dir);
    } else {
        ret = dmidecode_map(name); //TODO:
        if (!ret)
            ret = g_strdup("");
    }
    g_free(name);
    return ret;
}

int dmidecode_check_type(const gchar *path) {
    int ret = 0;
    gchar *name = g_path_get_basename(path);
    if (strcmp(name, "sysfs_map") == 0
        || strcmp(name, "--string") == 0
        || strcmp(name, "best_available") == 0 )
        ret = VSO_TYPE_DIR;
    g_free(name);
    if (!ret) {
        gchar *pp = g_path_get_dirname(path);
        gchar *pn = g_path_get_basename(pp);
        if (strcmp(pn, "sysfs_map") == 0)
            ret = VSO_TYPE_SYMLINK;
        else if (strcmp(pn, "--string") == 0)
            ret = VSO_TYPE_STRING;
        else if (strcmp(pn, "best_available") == 0)
            ret = VSO_TYPE_SYMLINK; //TODO:
        g_free(pp);
        g_free(pn);
    }
    return ret;
}

static sysobj_virt vol[] = {
    { .path = ":dmidecode",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .str = "--string\n" "sysfs_map\n" "best_available\n" "dmi_id",
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":dmidecode/--string",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "",
      .f_get_data = dmidecode_get_str, .f_get_type = dmidecode_check_type },
    { .path = ":dmidecode/sysfs_map",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "",
      .f_get_data = dmidecode_get_link, .f_get_type = dmidecode_check_type },
    { .path = ":dmidecode/best_available",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "",
      .f_get_data = dmidecode_get_best, .f_get_type = dmidecode_check_type },
    { .path = ":dmidecode/dmi_id",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/devices/virtual/dmi/id",
      .f_get_data = NULL },
};

void class_dmi_id() {
    int i = 0;
    while(tab_dmi_sysfs[i].id) {
        strcat(string_dir, tab_dmi_sysfs[i].id);
        strcat(string_dir, "\n");
        if (tab_dmi_sysfs[i].path) {
            strcat(sysfs_map_dir, tab_dmi_sysfs[i].id);
            strcat(sysfs_map_dir, "\n");
        }
        i++;
    }

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    cls_dmidecode = class_new();
    cls_dmidecode->tag = "dmidecode/string";
    cls_dmidecode->pattern = ":dmidecode/--string/*";
    cls_dmidecode->flags = OF_GLOB_PATTERN | OF_REQ_ROOT;
    class_add(cls_dmidecode);

    cls_dmi_id = class_new();
    cls_dmi_id->tag = "dmi/id";
    cls_dmi_id->pattern = "/sys/*/dmi/id*";
    cls_dmi_id->flags = OF_GLOB_PATTERN;
    cls_dmi_id->f_verify = dmi_id_verify;
    cls_dmi_id->f_label = dmi_id_label;
    cls_dmi_id->f_format = dmi_id_format;
    cls_dmi_id->f_flags = dmi_id_flags;
    class_add(cls_dmi_id);
}

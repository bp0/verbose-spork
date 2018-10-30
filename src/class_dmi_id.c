
#include "sysobj.h"

static sysobj_class *cls_dmi_id;

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
    if (fmt_opts & FMT_OPT_NO_JUNK && dmi_value_is_placeholder(obj) ) {
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

void class_dmi_id() {
    cls_dmi_id = class_new();
    cls_dmi_id->tag = "dmi/id";
    cls_dmi_id->pattern = "*/dmi/id*";
    cls_dmi_id->flags = OF_GLOB_PATTERN;
    cls_dmi_id->f_verify = dmi_id_verify;
    cls_dmi_id->f_label = dmi_id_label;
    cls_dmi_id->f_format = dmi_id_format;
    cls_dmi_id->f_flags = dmi_id_flags;
    class_add(cls_dmi_id);
}

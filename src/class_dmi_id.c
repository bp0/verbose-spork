
#include "sysobj.h"

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

static void ctype_class() {
    sysobj_class *c = g_new0(sysobj_class, 1);
    c->pattern = "dmi/id/chassis_type";
    c->s_label = _("Chassis Type");
    c->f_label = simple_label;
    c->f_format = chassis_type_str;
    c->flags = OF_NONE;
    class_add(c);
}

void class_dmi_id() {
    class_add_simple("dmi/id",                   _("Desktop Management Interface (DMI) product information"), OF_NONE);
    class_add_simple("dmi/id/bios_vendor",       _("BIOS Vendor"), OF_IS_VENDOR);
    class_add_simple("dmi/id/bios_version",      _("BIOS Version"), OF_NONE);
    class_add_simple("dmi/id/bios_date",         _("BIOS Date"), OF_NONE);
    class_add_simple("dmi/id/sys_vendor",        _("Product Vendor"), OF_IS_VENDOR);
    class_add_simple("dmi/id/product_name",      _("Product Name"), OF_NONE);
    class_add_simple("dmi/id/product_version",   _("Product Version"), OF_NONE);
    class_add_simple("dmi/id/product_serial",    _("Product Serial"), OF_REQ_ROOT);
    class_add_simple("dmi/id/product_uuid",      _("Product UUID"), OF_NONE);
    class_add_simple("dmi/id/product_sku",       _("Product SKU"), OF_NONE);
    class_add_simple("dmi/id/product_family",    _("Product Family"), OF_NONE);
    class_add_simple("dmi/id/board_vendor",      _("Board Vendor"), OF_IS_VENDOR);
    class_add_simple("dmi/id/board_name",        _("Board Name"), OF_NONE);
    class_add_simple("dmi/id/board_version",     _("Board Version"), OF_NONE);
    class_add_simple("dmi/id/board_serial",      _("Board Serial"), OF_REQ_ROOT);
    class_add_simple("dmi/id/board_asset_tag",   _("Board Asset Tag"), OF_NONE);
    class_add_simple("dmi/id/chassis_vendor",    _("Chassis Vendor"), OF_IS_VENDOR);
    class_add_simple("dmi/id/chassis_version",   _("Chassis Version"), OF_NONE);
    class_add_simple("dmi/id/chassis_serial",    _("Chassis Serial"), OF_REQ_ROOT);
    class_add_simple("dmi/id/chassis_asset_tag", _("Chassis Asset Tag"), OF_NONE);

    ctype_class();

}

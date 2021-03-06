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
#include "format_funcs.h"

gboolean dmi_id_verify(sysobj *obj);
const gchar *dmi_id_label(sysobj *obj);
gchar *dmi_id_format(sysobj *obj, int fmt_opts);
gchar *dmi_id_format_vendor(sysobj *obj, int fmt_opts);
guint dmi_id_flags(sysobj *obj, const sysobj_class *cls);

static attr_tab dmi_id_items[] = {
    { "bios_vendor",       N_("BIOS vendor"), OF_HAS_VENDOR, dmi_id_format_vendor },
    { "bios_version",      N_("BIOS version"), OF_NONE },
    { "bios_date",         N_("BIOS date"), OF_NONE },
    { "sys_vendor",        N_("product vendor"), OF_HAS_VENDOR, dmi_id_format_vendor },
    { "product_name",      N_("product name"), OF_NONE },
    { "product_version",   N_("product version"), OF_NONE },
    { "product_serial",    N_("product serial number"), OF_REQ_ROOT },
    { "product_uuid",      N_("product UUID"), OF_NONE },
    { "product_sku",       N_("product SKU"), OF_NONE },
    { "product_family",    N_("product family"), OF_NONE },
    { "board_vendor",      N_("board vendor"), OF_HAS_VENDOR, dmi_id_format_vendor },
    { "board_name",        N_("board name"), OF_NONE },
    { "board_version",     N_("board version"), OF_NONE },
    { "board_serial",      N_("board serial number"), OF_REQ_ROOT },
    { "board_asset_tag",   N_("board asset tag"), OF_NONE },
    { "chassis_vendor",    N_("chassis vendor"), OF_HAS_VENDOR, dmi_id_format_vendor },
    { "chassis_version",   N_("chassis version"), OF_NONE },
    { "chassis_serial",    N_("chassis serial number"), OF_REQ_ROOT },
    { "chassis_asset_tag", N_("chassis asset tag"), OF_NONE },
    ATTR_TAB_LAST
};

static vendor_list dmi_id_vendors(sysobj *obj) {
    return vendor_list_concat_va(4,
        sysobj_vendors_from_fn(obj->path, "sys_vendor"),
        sysobj_vendors_from_fn(obj->path, "chassis_vendor"),
        sysobj_vendors_from_fn(obj->path, "board_vendor"),
        sysobj_vendors_from_fn(obj->path, "bios_vendor") );
}

static sysobj_class cls_dmi_id[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "dmi:id", .pattern = "/sys/devices/virtual/dmi/id", .flags = OF_CONST | OF_HAS_VENDOR,
    .s_label = N_("Desktop Management Interface (DMI) product information"),
    .f_vendors = dmi_id_vendors },

  { SYSOBJ_CLASS_DEF
    .tag = "dmi:id:attr", .pattern = "/sys/devices/virtual/dmi/id/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_update_interval = UPDATE_INTERVAL_NEVER, .v_is_attr = TRUE,
    .attributes = dmi_id_items, .f_verify = dmi_id_verify, .f_format = dmi_id_format },

  { SYSOBJ_CLASS_DEF
    .tag = "dmidecode:string", .pattern = ":/extern/dmidecode/--string/*", .flags = OF_GLOB_PATTERN | OF_REQ_ROOT | OF_CONST,
    .s_label = N_("Result of `dmidecode --string {}`") },
};

static const gchar *chassis_types[] = {
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
    NULL,
};

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
        "System Serial Number",
        "Rev X.0x", /* ASUS board version nonsense */
        "x.x",      /* Gigabyte board version nonsense */
        "Not Specified",
        "NA",
        "SKU",

        /* noticed on an HP x360 with Insyde BIOS */
        "Type2 - Board Asset Tag",
        "Type1ProductConfigId",

        /* Toshiba Laptop with Insyde BIOS */
        "Base Board Version",
        "No Asset Tag",
        "None",
        "Type1Family",
        "123456789",

        NULL
    };

    /* only consider known items */
    i = attr_tab_lookup(dmi_id_items, obj->name);
    if (i == -1)
        return FALSE;

    v = (obj->data.was_read && obj->data.is_utf8) ? obj->data.str : NULL;
    if (v) {
        v = strdup(v);
        g_strchomp(v);

        i = 0;
        while(common[i]) {
            if (!strcasecmp(common[i], v))
                goto dmi_ignore_yes;
            i++;
        }

        chk = g_strdup(v);

        /* Zotac version nonsense */
        p = chk;
        while (*p != 0) { *p = 'x'; p++; } /* all X */
        if (!strcasecmp(chk, v))
            goto dmi_ignore_yes;
        p = chk;
        while (*p != 0) { *p = '0'; p++; } /* all 0 */
        if (!strcasecmp(chk, v))
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

gboolean dmi_id_verify(sysobj *obj) {
    if (SEQ(obj->name, "id")) {
        if (verify_parent_name(obj, "dmi"))
            return TRUE;
    } else if (verify_parent(obj, "/dmi/id")) {
        /* known items */
        if (verify_in_attr_tab(obj, dmi_id_items) )
            return TRUE;
        /* new unknown ones */
        if (g_str_has_prefix(obj->name, "sys_")
            || g_str_has_prefix(obj->name, "product_")
            || g_str_has_prefix(obj->name, "bios_")
            || g_str_has_prefix(obj->name, "chassis_")
            || g_str_has_prefix(obj->name, "board_") )
            return TRUE;
    }
    return FALSE;
}

gchar *dmi_id_format(sysobj *obj, int fmt_opts) {
    if (SEQ(obj->name, "chassis_type"))
        return sysobj_format_table(obj, (gchar**)&chassis_types, (int)G_N_ELEMENTS(chassis_types), 10, fmt_opts);

    if (dmi_value_is_placeholder(obj))
        return format_as_junk_value(obj->data.str, fmt_opts);

    return simple_format(obj, fmt_opts);
}

gchar *dmi_id_format_vendor(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    if (fmt_opts & FMT_OPT_ATERM
        || fmt_opts & FMT_OPT_PANGO
        || fmt_opts & FMT_OPT_HTML ) {
        gchar *val = g_strchomp(g_strdup(obj->data.str));
        const Vendor *v = vendor_match(val, NULL);
        if (v) {
            gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
            tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, fmt_opts);
            if (fmt_opts & FMT_OPT_PART)
                ret = g_strdup_printf("%s", ven_tag);
            else if (fmt_opts & FMT_OPT_SHORT)
                ret = g_strdup_printf("%s %s", ven_tag, v->name_short ? v->name_short : v->name);
            else
                ret = g_strdup_printf("%s %s", ven_tag, v->name);
            g_free(ven_tag);
        }
        g_free(val);
    }
    if (ret)
        return ret;
    return dmi_id_format(obj, fmt_opts);
}

void class_dmi_id() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_dmi_id); i++)
        class_add(&cls_dmi_id[i]);
}

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
#include <ctype.h>

const gchar mmc_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/mmc/mmc-dev-attrs.txt") "\n"
    BULLET REFLINK("https://lwn.net/Articles/682276/") "\n"
    "\n";

#define mk_oem_id(c1, c2) (c1 << 8 | c2)

/* source: https://www.cameramemoryspeed.com/sd-memory-card-faq/reading-sd-card-cid-serial-psn-internal-numbers/ */
static struct { unsigned int manfid, oemid; const char *vendor; } sd_ids[] = {
    { 0x000001, mk_oem_id('P','A'), "Panasonic" },
    { 0x000002, mk_oem_id('T','M'), "Toshiba" },
    { 0x000003, mk_oem_id('S','D'), "SanDisk" },
    { 0x000003, mk_oem_id('P','T'), "SanDisk" },
    { 0x00001b, mk_oem_id('S','M'), "Samsung" },
    { 0x00001d, mk_oem_id('A','D'), "ADATA" },
    { 0x000027, mk_oem_id('P','H'), "Phison" },
    { 0x000028, mk_oem_id('B','E'), "Lexar" },
    { 0x000031, mk_oem_id('S','P'), "Silicon Power" },
    { 0x000041, mk_oem_id('4','2'), "Kingston" },
    { 0x000074, mk_oem_id('J','E'), "Transcend" },
    { 0x000074, mk_oem_id('J','`'), "Transcend" },
    { 0x000076, 0xffff,             "Patriot" },
    { 0x000082, mk_oem_id('J','T'), "Sony" },
    { 0x00009c, mk_oem_id('S','O'), "Angelbird/Hoodman" },
    { 0x00009c, mk_oem_id('B','E'), "Angelbird/Hoodman" },
};

gchar *mmc_format_oemid(sysobj *obj, int fmt_opts) {
    unsigned int v = strtol(obj->data.str, NULL, 16);
    char c2 = v & 0xff, c1 = (v >> 8) & 0xff;
    unsigned int id = mk_oem_id(c1, c2);
    gchar *ret = g_strdup_printf("[0x%04x] \"%c%c\"", v, isprint(c1) ? c1 : '.', isprint(c2) ? c2 : '.' );
    const gchar *vstr = NULL;
    for(int i = 0; i < (int)G_N_ELEMENTS(sd_ids); i++) {
        if (id == sd_ids[i].oemid)
            vstr = sd_ids[i].vendor;
    }
    if (vstr) {
        gchar *ven_tag = vendor_match_tag(vstr, fmt_opts);
        ret = appf(ret, "%s", ven_tag ? ven_tag : vstr);
        g_free(ven_tag);
    }
    return ret;
}

gchar *mmc_format_manfid(sysobj *obj, int fmt_opts) {
    unsigned int id = strtol(obj->data.str, NULL, 16);
    const gchar *vstr = NULL;
    for(int i = 0; i < (int)G_N_ELEMENTS(sd_ids); i++) {
        if (id == sd_ids[i].manfid)
            vstr = sd_ids[i].vendor;
    }
    if (vstr) {
        gchar *ven_tag = vendor_match_tag(vstr, fmt_opts);
        gchar *ret = g_strdup_printf("[0x%06x] %s", id, ven_tag ? ven_tag : vstr);
        g_free(ven_tag);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

vendor_list mmc_vendor(sysobj *obj) {
    return vendor_list_concat(
        sysobj_vendors_from_fn(obj->path, "manfid"),
        sysobj_vendors_from_fn(obj->path, "oemid") );
}

vendor_list mmc_vendor_field(sysobj *obj) {
    if (!obj->data.str) return NULL;

    if (SEQ(obj->name, "oemid") ) {
        unsigned int v = strtol(obj->data.str, NULL, 16);
        char c2 = v & 0xff, c1 = (v >> 8) & 0xff;
        unsigned int id = mk_oem_id(c1, c2);
        const gchar *vstr = NULL;
        for(int i = 0; i < (int)G_N_ELEMENTS(sd_ids); i++) {
            if (id == sd_ids[i].oemid)
                vstr = sd_ids[i].vendor;
        }
        return vendor_list_append(NULL, vendor_match(vstr, NULL));
    }
    if (SEQ(obj->name, "manfid") ) {
        unsigned int id = strtol(obj->data.str, NULL, 16);
        const gchar *vstr = NULL;
        for(int i = 0; i < (int)G_N_ELEMENTS(sd_ids); i++) {
            if (id == sd_ids[i].manfid)
                vstr = sd_ids[i].vendor;
        }
        return vendor_list_append(NULL, vendor_match(vstr, NULL));
    }
}

static attr_tab mmc_items[] = {
    { "force_ro", N_("enforce read-only access even if write protect switch is off"), OF_NONE, fmt_1yes0no },
    { "cid",      N_("Card Identification Register") },
    { "csd",      N_("Card Specific Data Register") },
    { "scr",      N_("SD Card Configuration Register (SD only)") },
    { "date",     N_("manufacturing date (from CID Register)") },
    { "fwrev",    N_("firmware/product revision (from CID Register) (SD and MMCv1 only)") },
    { "hwrev",    N_("hardware/product revision (from CID Register) (SD and MMCv1 only)") },
    { "manfid",   N_("manufacturer ID (from CID Register)"), OF_HAS_VENDOR, mmc_format_manfid },
    { "name",     N_("product name (from CID Register)") },
    { "oemid",    N_("OEM/Application ID (from CID Register)"), OF_HAS_VENDOR, mmc_format_oemid },
    { "prv",      N_("product revision (from CID Register) (SD and MMCv4 only)") },
    { "serial",               N_("product serial number (from CID Register)") },
    { "erase_size",           N_("erase group size"), OF_NONE, fmt_bytes_to_higher },
    { "preferred_erase_size", N_("preferred erase size"), OF_NONE, fmt_bytes_to_higher },
    { "raw_rpmb_size_mult",   N_("RPMB partition size") },
    { "rel_sectors",          N_("reliable write sector count") },
    { "ocr",                  N_("Operation Conditions Register") },
    { "dsr",                  N_("Driver Stage Register") },
    { "cmdq_en",              N_("Command Queue enabled"), OF_NONE, fmt_1yes0no },
    { "type" }, //TODO: SD, SDIO, ...
    ATTR_TAB_LAST
};

vendor_list sdio_vendor(sysobj *obj) {
    return sysobj_vendors_from_fn(obj->path, "vendor");
}

vendor_list sdio_vendor_field(sysobj *obj) {
    if (obj->data.str && SEQ(obj->name, "vendor") ) {
        int vendor = strtol(obj->data.str, NULL, 16);
        gchar *vstr = auto_free( sysobj_raw_from_printf(":/lookup/sdio.ids/%04x/name", vendor) );
        return vendor_list_append(NULL, vendor_match(vstr, NULL) );
    }
    return simple_vendors(obj);
}

gchar *sdio_format_vdc(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    if (SEQ(obj->name, "vendor") ) {
        int vendor = strtol(obj->data.str, NULL, 16);
        gchar *vstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/sdio.ids/%04x/name", vendor);
        gchar *ven_tag = vendor_match_tag(vstr, fmt_opts);
        if (ven_tag) {
            g_free(vstr);
            vstr = ven_tag;
        }
        ret = (fmt_opts & FMT_OPT_PART)
            ? g_strdup(vstr ? vstr : _("Unknown"))
            : g_strdup_printf("[%04x] %s", vendor, vstr ? vstr : _("Unknown"));
        g_free(vstr);
    }
    if (SEQ(obj->name, "device") ) {
        int vendor = sysobj_uint32_from_fn(obj->path, "../vendor", 16);
        int device = strtol(obj->data.str, NULL, 16);
        gchar *dstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/sdio.ids/%04x/%04x/name", vendor, device);
        if (fmt_opts & FMT_OPT_PART) {
            if (!dstr)
                dstr = sysobj_format_from_fn(obj->path, "../class", fmt_opts | FMT_OPT_PART);
            ret = g_strdup(dstr ? dstr : _("Device"));
        } else
            ret = g_strdup_printf("[%04x] %s", device, dstr ? dstr : _("Device"));
        g_free(dstr);
    }
    if (SEQ(obj->name, "class") ) {
        int class = strtol(obj->data.str, NULL, 16);
        gchar *cstr = sysobj_format_from_printf(fmt_opts | FMT_OPT_OR_NULL, ":/lookup/sdio.ids/C %02x/name", class);
        ret = (fmt_opts & FMT_OPT_PART)
            ? g_strdup(cstr ? cstr : _("Unknown"))
            : g_strdup_printf("[%04x] %s", class, cstr ? cstr : _("Unknown"));
        g_free(cstr);
    }
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static attr_tab sdio_items[] = {
    { "vendor", N_("SD Association-assigned vendor ID"), OF_HAS_VENDOR, sdio_format_vdc },
    { "device", N_("vendor-specific device ID"), OF_NONE, sdio_format_vdc },
    { "class", N_("device class"), OF_NONE, sdio_format_vdc },
    ATTR_TAB_LAST
};

static sysobj_class cls_mmc[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "mmc", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .v_subsystem = "/sys/bus/mmc", .s_node_format = "{{type}}{{: |@vendors}}{{name}}",
    .s_halp = mmc_reference_markup_text, .f_vendors = mmc_vendor },
  { SYSOBJ_CLASS_DEF
    .tag = "mmc:attr", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/mmc", .attributes = mmc_items, .f_vendors = mmc_vendor_field,
    .s_halp = mmc_reference_markup_text },

  { SYSOBJ_CLASS_DEF
    .tag = "sdio", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST | OF_HAS_VENDOR,
    .v_subsystem = "/sys/bus/sdio", .s_node_format = "{{vendor}}{{device}}",
    /*.s_halp = sdio_reference_markup_text,*/ .f_vendors = sdio_vendor },
  { SYSOBJ_CLASS_DEF
    .tag = "sdio:attr", .pattern = "/sys/devices/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/sdio", .attributes = sdio_items, .f_vendors = sdio_vendor_field,
    /*.s_halp = sdio_reference_markup_text*/ },
};

void class_mmc() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_mmc); i++)
        class_add(&cls_mmc[i]);
}

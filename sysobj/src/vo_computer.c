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

static sysobj_class *cls_mobo;

static const struct { gchar *rp; gchar *lbl; int extra_flags; } mobo_items[] = {
    { "board_name",  N_("System Board"), OF_NONE },
    { NULL, NULL, 0 }
};

int mobo_lookup(const gchar *key) {
    int i = 0;
    while(mobo_items[i].rp) {
        if (strcmp(key, mobo_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

gboolean mobo_verify(sysobj *obj) {
    if (!strcmp(obj->name, "board")) {
        if (verify_parent_name(obj, ":computer"))
            return TRUE;
    } else if (verify_parent(obj, ":computer/board"))
        return TRUE;
    return FALSE;
}

const gchar *mobo_label(sysobj *obj) {
    int i = mobo_lookup(obj->name);
    if (i != -1)
        return _(mobo_items[i].lbl);
    return NULL;
}

gchar *mobo_format(sysobj *obj, int fmt_opts) {
    if (!strcmp(obj->name, "board")) {
        return sysobj_format_from_fn(obj->path, "board_name", fmt_opts );
    }
    return simple_format(obj, fmt_opts);
}

/* remember to handle obj == NULL */
guint mobo_flags(sysobj *obj) {
    if (obj) {
        int i = mobo_lookup(obj->name);
        if (i != -1)
            return obj->cls->flags | mobo_items[i].extra_flags;
    }
    return cls_mobo ? cls_mobo->flags : OF_NONE;
}

#define dt_get_model() sysobj_format_from_fn(":devicetree/base", "model", FMT_OPT_OR_NULL );
#define dmi_get_str(k) sysobj_format_from_fn(":dmidecode/best_available", k, FMT_OPT_NO_JUNK | FMT_OPT_OR_NULL );
gchar *mobo_get_name(const gchar *path) {
    gchar *board_name, *board_vendor, *board_version;
    gchar *product_name, *product_vendor, *product_version;
    gchar *board_part = NULL, *product_part = NULL;
    //const gchar *tmp;
    int b = 0, p = 0;

    PARAM_NOT_UNUSED(path);

    gchar *ret = NULL;

    /* use device tree "model" */
    ret = dt_get_model();
    if (ret)
        goto got_mobo_ok;

    /* use DMI */
    board_name = dmi_get_str("baseboard-product-name");
    board_version = dmi_get_str("baseboard-version");
    board_vendor = dmi_get_str("baseboard-manufacturer");
    if (board_vendor) {
        /* attempt to shorten */ /*
        tmp = vendor_get_shortest_name(board_vendor);
        if (tmp && tmp != board_vendor) {
            g_free(board_vendor);
            board_vendor = g_strdup(tmp);
        } */
    }

    product_name = dmi_get_str("system-product-name");
    product_version = dmi_get_str("system-version");
    product_vendor = dmi_get_str("system-manufacturer");
    if (product_vendor) {
        /* attempt to shorten */ /*
        tmp = vendor_get_shortest_name(product_vendor);
        if (tmp && tmp != product_vendor) {
            g_free(product_vendor);
            product_vendor = g_strdup(tmp);
        } */
    }

    if (board_vendor && product_vendor &&
        strcmp(board_vendor, product_vendor) == 0) {
            /* ignore duplicate vendor */
            g_free(product_vendor);
            product_vendor = NULL;
    }

    if (board_name && product_name &&
        strcmp(board_name, product_name) == 0) {
            /* ignore duplicate name */
            g_free(product_name);
            product_name = NULL;
    }

    if (board_version && product_version &&
        strcmp(board_version, product_version) == 0) {
            /* ignore duplicate version */
            g_free(product_version);
            product_version = NULL;
    }

    if (board_name) b += 1;
    if (board_vendor) b += 2;
    if (board_version) b += 4;

    switch(b) {
        case 1: /* only name */
            board_part = g_strdup(board_name);
            break;
        case 2: /* only vendor */
            board_part = g_strdup(board_vendor);
            break;
        case 3: /* only name and vendor */
            board_part = g_strdup_printf("%s %s", board_vendor, board_name);
            break;
        case 4: /* only version? Seems unlikely */
            board_part = g_strdup(board_version);
            break;
        case 5: /* only name and version? */
            board_part = g_strdup_printf("%s %s", board_name, board_version);
            break;
        case 6: /* only vendor and version (like lpereira's Thinkpad) */
            board_part = g_strdup_printf("%s %s", board_vendor, board_version);
            break;
        case 7: /* all */
            board_part = g_strdup_printf("%s %s %s", board_vendor, board_name, board_version);
            break;
    }

    if (product_name) p += 1;
    if (product_vendor) p += 2;
    if (product_version) p += 4;

    switch(p) {
        case 1: /* only name */
            product_part = g_strdup(product_name);
            break;
        case 2: /* only vendor */
            product_part = g_strdup(product_vendor);
            break;
        case 3: /* only name and vendor */
            product_part = g_strdup_printf("%s %s", product_vendor, product_name);
            break;
        case 4: /* only version? Seems unlikely */
            product_part = g_strdup(product_version);
            break;
        case 5: /* only name and version? */
            product_part = g_strdup_printf("%s %s", product_name, product_version);
            break;
        case 6: /* only vendor and version? */
            product_part = g_strdup_printf("%s %s", product_vendor, product_version);
            break;
        case 7: /* all */
            product_part = g_strdup_printf("%s %s %s", product_vendor, product_name, product_version);
            break;
    }

    if (board_part && product_part) {
        ret = g_strdup_printf("%s (%s)", board_part, product_part);
        g_free(board_part);
        g_free(product_part);
    } else if (board_part)
        ret = board_part;
    else if (product_part)
        ret = product_part;
    else
        ret = g_strdup(_("(Unknown)"));

    g_free(board_name);
    g_free(board_vendor);
    g_free(board_version);
    g_free(product_name);
    g_free(product_vendor);
    g_free(product_version);

    if (!ret)
        ret = g_strdup(_("Unknown"));

got_mobo_ok:
    return ret;
}

static sysobj_virt vol[] = {
    { .path = ":computer",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .str = "*",
      .f_get_data = NULL },
    { .path = ":computer/board",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .str = "*",
      .f_get_data = NULL },
    { .path = ":computer/board/board_name",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .str = "",
      .f_get_data = mobo_get_name },
    { .path = ":computer/board/dmi_id",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/devices/virtual/dmi/id",
      .f_get_data = NULL },
  { .path = ":computer/board/dt_model",
      .type = VSO_TYPE_SYMLINK | VSO_TYPE_CONST,
      .str = ":devicetree/base/model",
      .f_get_data = NULL },
  { .path = ":computer/pci",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/bus/pci/devices",
      .f_get_data = NULL },
  { .path = ":computer/usb",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/bus/usb/devices",
      .f_get_data = NULL },
  { .path = ":computer/usb",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/bus/usb/devices",
      .f_get_data = NULL },
  { .path = ":computer/batteries",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .str = "/sys/class/power_supply",
      .f_get_data = NULL },
};

void vo_computer() {
    /* add virtual sysobj */
    int i;
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    cls_mobo = class_new();
    cls_mobo->tag = "compinfo";
    cls_mobo->pattern = ":computer*";
    cls_mobo->flags = OF_GLOB_PATTERN;
    cls_mobo->f_verify = mobo_verify;
    cls_mobo->f_label = mobo_label;
    cls_mobo->f_format = mobo_format;
    cls_mobo->f_flags = mobo_flags;
    class_add(cls_mobo);
}

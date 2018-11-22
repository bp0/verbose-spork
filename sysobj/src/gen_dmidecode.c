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

static gchar sysfs_map_dir[1024] = "";
static gchar string_dir[1024] = "";

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

static gchar *dmidecode_map(const gchar *name) {
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

static gchar *util_exec(const gchar **argv) {
    gboolean spawned = FALSE;
    gchar *out = NULL, *err = NULL, *ret = NULL;
    int status = 0;
    spawned = g_spawn_sync(NULL, (gchar**)argv, NULL, G_SPAWN_DEFAULT, NULL, NULL,
            &out, &err, &status, NULL);
    if (spawned) {
        if (status == 0)
            ret = out;
        else
            g_free(out);

        g_free(err);
    }
    return ret;
}

static gchar *dmidecode_get_str(const gchar *path) {
    gchar *name = g_path_get_basename(path);
    gchar *ret = NULL;
    if (strcmp(name, "--string") == 0) {
        ret = g_strdup(string_dir);
    } else {
        /* only when not using a different sysobj_root */
        if (!sysobj_using_alt_root() ) {
            const gchar *argv[] = { "/usr/bin/env", "dmidecode", "-s", name, NULL };
            ret = util_exec(argv);
        }
    }
    g_free(name);
    return ret;
}

static gchar *dmidecode_get_link(const gchar *path) {
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

static gchar *dmidecode_get_best(const gchar *path) {
    gchar *name = g_path_get_basename(path);
    gchar *ret = NULL;
    if (strcmp(name, "best_available") == 0) {
        ret = g_strdup(string_dir);
    } else {
        ret = dmidecode_map(name);
        if (!ret)
            ret = g_strdup_printf("%s/%s", ":/dmidecode/--string", name);
    }
    g_free(name);
    return ret;
}

static int dmidecode_check_type(const gchar *path) {
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
            ret = VSO_TYPE_STRING | VSO_TYPE_REQ_ROOT;
        else if (strcmp(pn, "best_available") == 0)
            ret = VSO_TYPE_SYMLINK;
        g_free(pp);
        g_free(pn);
    }
    return ret;
}

static sysobj_virt vol[] = {
    { .path = ":/dmidecode", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":/dmidecode/--string", .str = "",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = dmidecode_get_str, .f_get_type = dmidecode_check_type },
    { .path = ":/dmidecode/sysfs_map", .str = "",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = dmidecode_get_link, .f_get_type = dmidecode_check_type },
    { .path = ":/dmidecode/best_available", .str = "",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = dmidecode_get_best, .f_get_type = dmidecode_check_type },
    { .path = ":/dmidecode/dmi_id", .str = "/sys/devices/virtual/dmi/id",
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

void gen_dmidecode() {
    int i = 0;
    /* create directories */
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
}

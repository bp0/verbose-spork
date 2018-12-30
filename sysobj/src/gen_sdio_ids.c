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

/* Generator for ids from the sdio.ids file (https://github.com/systemd/systemd/blob/master/hwdb/sdio.ids)
 *  :/lookup/sdio.ids
 *
 * Items are generated on-demand and cached.
 *
 * :/lookup/sdio.ids/<vendor>/name
 * :/lookup/sdio.ids/<vendor>/<device>/name
 * :/lookup/sdio.ids/class/<class>/name
 *
 */
#include "sysobj.h"

#define sdio_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

#define SDIO_ILLEGAL_PART 0xffff
#define SDIO_ID_BUFF_SIZE 128
typedef struct {
    gchar strs[SDIO_ID_BUFF_SIZE * 3];
    gchar *vendor_str;
    gchar *device_str;
    gchar *dev_class_str;
    int vendor;
    int device;
    int dev_class;
} sdio_id;
#define sdio_id_new() g_new0(sdio_id, 1)
#define sdio_id_free(aid) g_free(aid)

sdio_id *scan_sdio_ids_file(int vendor, int device, int dev_class) {
    char buff[SDIO_ID_BUFF_SIZE] = "",
         imp[SDIO_ID_BUFF_SIZE] = "",
         prt[SDIO_ID_BUFF_SIZE] = "",
         *cls = NULL, *p;
    FILE *fd;
    int tabs, imp_match = 0;
    unsigned id;

    sdio_id *ret = NULL;

    gchar *sdio_ids_file = sysobj_find_data_file("sdio.ids");
    if (!sdio_ids_file) {
        sdio_msg("sdio.ids file not found");
        return ret;
    }

    fd = fopen(sdio_ids_file, "r");
    g_free(sdio_ids_file);
    if (!fd) {
        sdio_msg("sdio.ids file could not be read");
        return NULL;
    }

    int class_section = 0;
    while (fgets(buff, SDIO_ID_BUFF_SIZE, fd)) {
        /* line ends at comment */
        p = strchr(buff, '#');
        if (p) *p = 0;

        /* trim trailing white space */
        p = buff + strlen(buff) - 1;
        while(p > buff && isspace((unsigned char)*p)) p--;
        *(p+1) = 0;
        p = buff;

        if (buff[0] == 0)    continue; /* empty line */
        if (buff[0] == '\n') continue; /* empty line */
        if (buff[0] == 'C'
            && buff[1] == ' ')
            class_section = 1; /* arrived at dev_class section */

        /* scan for fields */
        if (class_section) {
            if (dev_class == -1)
                goto scan_sdio_id_done;
            id = strtol(p + 2, &p, 16);
            while(isspace((unsigned char)*p)) p++;
            if (id == dev_class) {
                cls = p;
                goto scan_sdio_id_done;
            }
            if (id > dev_class)
                goto scan_sdio_id_done;
            continue;
        }

        tabs = 0;
        while(*p == '\t') { tabs++; p++; }
        id = strtol(p, &p, 16);
        while(isspace((unsigned char)*p)) p++;
        if (tabs == 0) {
            /* vendor */
            if (id == vendor) {
                strcpy(imp, p);
                imp_match = 1;
            } else if (id > vendor && dev_class == -1)
                goto scan_sdio_id_done;
        } else if (tabs == 1 && imp_match) {
            /* device */
            if (id == device) {
                strcpy(prt, p);
                if (dev_class == -1)
                    goto scan_sdio_id_done;
            } else if (id > device && dev_class == -1)
                goto scan_sdio_id_done;
        }
    }

scan_sdio_id_done:
    fclose(fd);

    if (imp_match || cls) {
        ret = sdio_id_new();
        ret->vendor = vendor;
        ret->device = device;
        ret->dev_class = dev_class;
        ret->vendor_str = ret->strs;
        ret->device_str = ret->strs + strlen(imp) + 1;
        ret->dev_class_str = ret->device_str + strlen(prt) + 1;
        strcpy(ret->vendor_str, imp);
        strcpy(ret->device_str, prt);
        if (cls)
            strcpy(ret->dev_class_str, cls);
    }
    return ret;
}

static void gen_sdio_ids_cache_item(sdio_id *pid) {
    gchar buff[128] = "";
    int dev_symlink_flags = VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN;
    if (pid->vendor != -1) {
        sprintf(buff, ":/lookup/sdio.ids/%04x", pid->vendor);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->vendor_str)
            sysobj_virt_add_simple(buff, "name", pid->vendor_str, VSO_TYPE_STRING );
    }
    if (pid->device != -1) {
        sprintf(buff, ":/lookup/sdio.ids/%04x/%04x", pid->vendor, pid->device);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->device_str)
            sysobj_virt_add_simple(buff, "name", pid->device_str, VSO_TYPE_STRING );
    }
    if (pid->dev_class != -1) {
        sprintf(buff, ":/lookup/sdio.ids/class/%02x", pid->dev_class & 0xff);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->vendor_str)
            sysobj_virt_add_simple(buff, "name", pid->dev_class_str, VSO_TYPE_STRING );
    }
}

static void buff_basename(const gchar *path, gchar *buff, gsize n) {
    gchar *fname = g_path_get_basename(path);
    strncpy(buff, fname, n);
    g_free(fname);
}

static gboolean name_is_0x02(gchar *name) {
    if (!name) return FALSE;
    if (  isxdigit(name[0])
        && isxdigit(name[1])
        && name[2] == 0 )
        return TRUE;
    return FALSE;
}

static gboolean name_is_0x04(gchar *name) {
    if (!name) return FALSE;
    if (  isxdigit(name[0])
        && isxdigit(name[1])
        && isxdigit(name[2])
        && isxdigit(name[3])
        && name[4] == 0 )
        return TRUE;
    return FALSE;
}

gboolean sdio_ids_lookup(sdio_id **pid) {
    sdio_id *aid = scan_sdio_ids_file((*pid)->vendor, (*pid)->device, (*pid)->dev_class);
    if (aid) {
        sdio_id_free(*pid);
        *pid = aid;
        return TRUE;
    }
    return FALSE;
}

static gchar *gen_sdio_ids_lookup_value(const gchar *path) {
    if (!path) return NULL;
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "sdio.ids") )
        return g_strdup("*");

    sdio_id *pid = sdio_id_new();
    pid->dev_class = -1;
    int32_t name_id = -1;
    if (name_is_0x04(name))
        name_id = strtol(name, NULL, 16);

    int mc = sscanf(path, ":/lookup/sdio.ids/%04x/%04x", &pid->vendor, &pid->device);
    gchar *verify = NULL;
    switch(mc) {
        case 2:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/sdio.ids/%04x/%04x/%s", pid->vendor, pid->device, name)
                : g_strdup_printf(":/lookup/sdio.ids/%04x/%04x", pid->vendor, pid->device);
        case 1:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/sdio.ids/%04x/%s", pid->vendor, name)
                : g_strdup_printf(":/lookup/sdio.ids/%04x", pid->vendor);

            if (strcmp(path, verify) != 0) {
                sdio_id_free(pid);
                return NULL;
            }

            int ok = sdio_ids_lookup(&pid);
            gen_sdio_ids_cache_item(pid);
            break;
        case 0:
            sdio_id_free(pid);
            return NULL;
    }

    if (name_id != -1) {
        sdio_id_free(pid);
        return "*";
    }

    gchar *ret = NULL;
    if (SEQ(name, "name") ) {
        switch(mc) {
            case 1: ret = g_strdup(pid->vendor_str); break;
            case 2: ret = g_strdup(pid->device_str); break;
        }
    }
    sdio_id_free(pid);
    return ret;
}

static gchar *gen_sdio_ids_lookup_class_value(const gchar *path) {
    if (!path) return NULL;
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(path, ":/lookup/sdio.ids/class") )
        return g_strdup("*");

    sdio_id *pid = g_new0(sdio_id, 1);
    pid->vendor = -1;
    pid->device = -1;
    int32_t name_id = -1;
    if ( name_is_0x04(name) )
        name_id = strtol(name, NULL, 16);

    int mc = sscanf(path, ":/lookup/sdio.ids/class/%02x", &pid->dev_class);
    gchar *verify = NULL;
    switch(mc) {
        case 1:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/lookup/sdio.ids/class/%02x/%s", pid->dev_class, name)
                : g_strdup_printf(":/lookup/sdio.ids/class/%02x", pid->dev_class);

            if (strcmp(path, verify) != 0) {
                sdio_id_free(pid);
                return NULL;
            }

            int ok = sdio_ids_lookup(&pid);
            gen_sdio_ids_cache_item(pid);
            break;
        case 0:
            sdio_id_free(pid);
            return NULL;
    }

    gchar *ret = NULL;
    if (SEQ(name, "name") )
        ret = g_strdup(pid->dev_class_str);

    sdio_id_free(pid);
    return ret;
}

static int gen_sdio_ids_lookup_type(const gchar *path) {
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "sdio.ids") )
        return VSO_TYPE_BASE;

    if (SEQ(name, "class") )
        return VSO_TYPE_BASE;

    if (SEQ(name, "name") )
        return VSO_TYPE_STRING;

    if (name_is_0x04(name) || name_is_0x02(name))
        return VSO_TYPE_DIR;

    return VSO_TYPE_NONE;
}

static sysobj_virt vol[] = {
    { .path = ":/lookup/sdio.ids/class", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = gen_sdio_ids_lookup_class_value, .f_get_type = gen_sdio_ids_lookup_type },
    { .path = ":/lookup/sdio.ids", .str = "*",
      .type =  VSO_TYPE_DIR | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = gen_sdio_ids_lookup_value, .f_get_type = gen_sdio_ids_lookup_type },
};

void gen_sdio_ids() {
    /* add virtual sysobj */
    for (int i = 0; i < (int)G_N_ELEMENTS(vol); i++)
        sysobj_virt_add(&vol[i]);
}

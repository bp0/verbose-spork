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

/* Generator for ids from the arm.ids file (https://github.com/bp0/armids)
 *  :/procs/cpuinfo/arm.ids
 *
 * Items are generated on-demand and cached.
 *
 * :/procs/cpuinfo/arm.ids/<implementer>/name
 * :/procs/cpuinfo/arm.ids/<implementer>/<part>/name
 *
 */
#include "sysobj.h"
#include "arm_data.h"

static void gen_arm_ids_cache_item(arm_id *pid) {
    gchar buff[128] = "";
    int dev_symlink_flags = VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN;
    if (pid->implementer) {
        sprintf(buff, ":/procs/cpuinfo/arm.ids/%02x", pid->implementer);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->implementer_str)
            sysobj_virt_add_simple(buff, "name", pid->implementer_str, VSO_TYPE_STRING );
    }
    if (pid->part) {
        sprintf(buff, ":/procs/cpuinfo/arm.ids/%02x/%03x", pid->implementer, pid->part);
        sysobj_virt_add_simple(buff, NULL, "*", VSO_TYPE_DIR );
        if (pid->part_str)
            sysobj_virt_add_simple(buff, "name", pid->part_str, VSO_TYPE_STRING );
    }
}

#define arm_msg(...) /* fprintf (stderr, __VA_ARGS__) */

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

static gboolean name_is_0x03(gchar *name) {
    if (!name) return FALSE;
    if (  isxdigit(name[0])
        && isxdigit(name[1])
        && isxdigit(name[2])
        && name[3] == 0 )
        return TRUE;
    return FALSE;
}

gboolean arm_ids_lookup(arm_id **pid) {
    arm_id *aid = scan_arm_ids_file((*pid)->implementer, (*pid)->part);
    if (aid) {
        arm_id_free(*pid);
        *pid = aid;
        return TRUE;
    }
    return FALSE;
}

static gchar *gen_arm_ids_lookup_value(const gchar *path) {
    if (!path) return NULL;
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "arm.ids") )
        return g_strdup("*");

    arm_id *pid = arm_id_new();
    int32_t name_id = -1;
    if ( name_is_0x02(name) || name_is_0x03(name) )
        name_id = strtol(name, NULL, 16);

    int mc = sscanf(path, ":/procs/cpuinfo/arm.ids/%02x/%03x", &pid->implementer, &pid->part);
    gchar *verify = NULL;
    switch(mc) {
        case 2:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/procs/cpuinfo/arm.ids/%02x/%03x/%s", pid->implementer, pid->part, name)
                : g_strdup_printf(":/procs/cpuinfo/arm.ids/%02x/%03x", pid->implementer, pid->part);
        case 1:
            if (!verify)
            verify = (name_id == -1)
                ? g_strdup_printf(":/procs/cpuinfo/arm.ids/%02x/%s", pid->implementer, name)
                : g_strdup_printf(":/procs/cpuinfo/arm.ids/%02x", pid->implementer);

            if (strcmp(path, verify) != 0) {
                arm_id_free(pid);
                return NULL;
            }

            int ok = arm_ids_lookup(&pid);
            gen_arm_ids_cache_item(pid);
            break;
        case 0:
            arm_id_free(pid);
            return NULL;
    }

    if (name_id != -1) {
        arm_id_free(pid);
        return "*";
    }

    gchar *ret = NULL;
    if (SEQ(name, "name") ) {
        switch(mc) {
            case 1: ret = g_strdup(pid->implementer_str); break;
            case 2: ret = g_strdup(pid->part_str); break;
        }
    }
    arm_id_free(pid);
    return ret;
}

static int gen_arm_ids_lookup_type(const gchar *path) {
    gchar name[16] = "";
    buff_basename(path, name, 15);

    if (SEQ(name, "arm.ids") )
        return VSO_TYPE_DIR | VSO_TYPE_DYN;

    if (SEQ(name, "name") )
        return VSO_TYPE_STRING;

    if (name_is_0x02(name) || name_is_0x03(name))
        return VSO_TYPE_DIR;

    return VSO_TYPE_NONE;
}

void gen_arm_ids() {
    sysobj_virt *vo = sysobj_virt_new();
    vo->path = g_strdup(":/procs/cpuinfo/arm.ids");
    vo->str = g_strdup("*");
    vo->type = VSO_TYPE_DIR | VSO_TYPE_DYN;
    vo->f_get_data = gen_arm_ids_lookup_value;
    vo->f_get_type = gen_arm_ids_lookup_type;
    sysobj_virt_add(vo);
}

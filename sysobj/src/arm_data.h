/*
 * rpiz - https://github.com/bp0/rpiz
 * Copyright (C) 2017  Burt P. <pburt0@gmail.com>
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

#ifndef _ARMDATA_H_
#define _ARMDATA_H_

/* fixes arch string */
const char *arm_arch(const char *cpuinfo_arch_str);
const char *arm_arch_more(const char *cpuinfo_arch_str);

gchar *arm_implementer(const gchar *imp);
gchar *arm_part(const gchar *imp, const gchar *part);
gchar *arm_decoded_name(
    const gchar *imp, const gchar *part,
    const gchar *var, const gchar *rev,
    const gchar *arch);

/* cpu flags from /proc/cpuinfo */
const char *arm_flag_list(void);                  /* list of all known flags */
const char *arm_flag_meaning(const char *flag);  /* lookup flag meaning */

#define ARM_ILLEGAL_PART 0xffff
#define ARM_ID_BUFF_SIZE 128
typedef struct {
    gchar strs[ARM_ID_BUFF_SIZE * 2];
    gchar *implementer_str;
    gchar *part_str;
    int implementer;
    int part;
} arm_id;
#define arm_id_new() g_new0(arm_id, 1)
#define arm_id_free(aid) g_free(aid)

arm_id *scan_arm_ids_file(unsigned implementer, unsigned part);

#endif

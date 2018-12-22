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

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sysobj.h"
#include "arm_data.h"

#ifndef C_
#define C_(Ctx, String) String
#endif
#ifndef NC_
#define NC_(Ctx, String) String
#endif

/* sources:
 *   https://unix.stackexchange.com/a/43563
 *   git:linux/arch/arm/kernel/setup.c
 *   git:linux/arch/arm64/kernel/cpuinfo.c
 */
static struct {
    char *name, *meaning;
} tab_flag_meaning[] = {
    /* arm/hw_cap */
    { "swp",      NC_("arm-flag", /*/flag:swp*/      "SWP instruction (atomic read-modify-write)") },
    { "half",     NC_("arm-flag", /*/flag:half*/     "Half-word loads and stores") },
    { "thumb",    NC_("arm-flag", /*/flag:thumb*/    "Thumb (16-bit instruction set)") },
    { "26bit",    NC_("arm-flag", /*/flag:26bit*/    "26-Bit Model (Processor status register folded into program counter)") },
    { "fastmult", NC_("arm-flag", /*/flag:fastmult*/ "32x32->64-bit multiplication") },
    { "fpa",      NC_("arm-flag", /*/flag:fpa*/      "Floating point accelerator") },
    { "vfp",      NC_("arm-flag", /*/flag:vfp*/      "VFP (early SIMD vector floating point instructions)") },
    { "edsp",     NC_("arm-flag", /*/flag:edsp*/     "DSP extensions (the 'e' variant of the ARM9 CPUs, and all others above)") },
    { "java",     NC_("arm-flag", /*/flag:java*/     "Jazelle (Java bytecode accelerator)") },
    { "iwmmxt",   NC_("arm-flag", /*/flag:iwmmxt*/   "SIMD instructions similar to Intel MMX") },
    { "crunch",   NC_("arm-flag", /*/flag:crunch*/   "MaverickCrunch coprocessor (if kernel support enabled)") },
    { "thumbee",  NC_("arm-flag", /*/flag:thumbee*/  "ThumbEE") },
    { "neon",     NC_("arm-flag", /*/flag:neon*/     "Advanced SIMD/NEON on AArch32") },
    { "evtstrm",  NC_("arm-flag", /*/flag:evtstrm*/  "Kernel event stream using generic architected timer") },
    { "vfpv3",    NC_("arm-flag", /*/flag:vfpv3*/    "VFP version 3") },
    { "vfpv3d16", NC_("arm-flag", /*/flag:vfpv3d16*/ "VFP version 3 with 16 D-registers") },
    { "vfpv4",    NC_("arm-flag", /*/flag:vfpv4*/    "VFP version 4 with fast context switching") },
    { "vfpd32",   NC_("arm-flag", /*/flag:vfpd32*/   "VFP with 32 D-registers") },
    { "tls",      NC_("arm-flag", /*/flag:tls*/      "TLS register") },
    { "idiva",    NC_("arm-flag", /*/flag:idiva*/    "SDIV and UDIV hardware division in ARM mode") },
    { "idivt",    NC_("arm-flag", /*/flag:idivt*/    "SDIV and UDIV hardware division in Thumb mode") },
    { "lpae",     NC_("arm-flag", /*/flag:lpae*/     "40-bit Large Physical Address Extension") },
    /* arm/hw_cap2 */
    { "pmull",    NC_("arm-flag", /*/flag:pmull*/    "64x64->128-bit F2m multiplication (arch>8)") },
    { "aes",      NC_("arm-flag", /*/flag:aes*/      "Crypto:AES (arch>8)") },
    { "sha1",     NC_("arm-flag", /*/flag:sha1*/     "Crypto:SHA1 (arch>8)") },
    { "sha2",     NC_("arm-flag", /*/flag:sha2*/     "Crypto:SHA2 (arch>8)") },
    { "crc32",    NC_("arm-flag", /*/flag:crc32*/    "CRC32 checksum instructions (arch>8)") },
    /* arm64/hw_cap */
    { "fp",       NULL },
    { "asimd",    NC_("arm-flag", /*/flag:asimd*/    "Advanced SIMD/NEON on AArch64 (arch>8)") },
    { "atomics",  NULL },
    { "fphp",     NULL },
    { "asimdhp",  NULL },
    { "cpuid",    NULL },
    { "asimdrdm", NULL },
    { "jscvt",    NULL },
    { "fcma",     NULL },
    { "lrcpc",    NULL },
    { NULL, NULL }
};

static struct {
    char *code; char *name; char *more;
} tab_arm_arch[] = {
    { "7",	"AArch32",	"AArch32 (ARMv7)" },
    { "8",	"AArch64",	"AArch64 (ARMv8)" },
    { "AArch32",	"AArch32",	"AArch32 (ARMv7)" },
    { "AArch64",	"AArch64",	"AArch64 (ARMv8)" },
    { NULL, NULL, NULL },
};

static char all_flags[1024] = "";

#define APPEND_FLAG(f) strcat(all_flags, f); strcat(all_flags, " ");
const char *arm_flag_list() {
    int i = 0, built = 0;
    built = strlen(all_flags);
    if (!built) {
        while(tab_flag_meaning[i].name != NULL) {
            APPEND_FLAG(tab_flag_meaning[i].name);
            i++;
        }
    }
    return all_flags;
}

const char *arm_flag_meaning(const char *flag) {
    int i = 0;
    if (flag)
    while(tab_flag_meaning[i].name != NULL) {
        if (strcmp(tab_flag_meaning[i].name, flag) == 0) {
            if (tab_flag_meaning[i].meaning != NULL)
                return C_("arm-flag", tab_flag_meaning[i].meaning);
            else return NULL;
        }
        i++;
    }
    return NULL;
}

const char *arm_arch(const char *cpuinfo_arch_str) {
    int i = 0;
    if (cpuinfo_arch_str)
    while(tab_arm_arch[i].code) {
        if (strcmp(tab_arm_arch[i].code, cpuinfo_arch_str) == 0)
            return tab_arm_arch[i].name;
        i++;
    }
    return cpuinfo_arch_str;
}

const char *arm_arch_more(const char *cpuinfo_arch_str) {
    int i = 0;
    if (cpuinfo_arch_str)
    while(tab_arm_arch[i].code) {
        if (strcmp(tab_arm_arch[i].code, cpuinfo_arch_str) == 0)
            return tab_arm_arch[i].more;
        i++;
    }
    return cpuinfo_arch_str;
}

#define arm_msg(...) fprintf (stderr, __VA_ARGS__)

arm_id *scan_arm_ids_file(unsigned implementer, unsigned part) {
    char buff[ARM_ID_BUFF_SIZE];
    char imp[ARM_ID_BUFF_SIZE] = "", *prt = NULL, *p;
    FILE *fd;
    int tabs, imp_match = 0;
    unsigned id;

    arm_id *ret = NULL;

    gchar *arm_ids_file = sysobj_find_data_file("arm.ids");
    if (!arm_ids_file) {
        arm_msg("arm.ids file not found\n");
        return ret;
    }

    fd = fopen(arm_ids_file, "r");
    g_free(arm_ids_file);
    if (!fd) {
        arm_msg("arm.ids file could not be read\n");
        return NULL;
    }

    while (fgets(buff, ARM_ID_BUFF_SIZE, fd)) {
        /* line ends at comment */
        p = strchr(buff, '#');
        if (p) *p = 0;

        /* trim trailing white space */
        p = buff + strlen(buff) - 1;
        while(p > buff && isspace((unsigned char)*p)) p--;
        *(p+1) = 0;

        /* scan for fields */
        tabs = 0;
        p = buff;
        while(*p == '\t') { tabs++; p++; }
        id = strtol(p, &p, 16);
        while(isspace((unsigned char)*p)) p++;
        if (tabs == 0) {
            /* implementer */
            if (id == implementer) {
                strcpy(imp, p);
                imp_match = 1;
            } else if (id > implementer)
                goto scan_arm_id_done;
        } else if (tabs == 1 && imp_match) {
            /* part */
            if (id == part) {
                prt = p;
                goto scan_arm_id_done;
            } else if (id > part)
                goto scan_arm_id_done;
        }
    }

scan_arm_id_done:
    fclose(fd);

    if (imp_match) {
        ret = arm_id_new();
        ret->implementer = implementer;
        ret->part = part;
        ret->implementer_str = ret->strs;
        ret->part_str = ret->strs + strlen(imp) + 1;
        strcpy(ret->implementer_str, imp);
        if (prt)
            strcpy(ret->part_str, prt);
    }
    return ret;
}

gchar *arm_implementer(const gchar *imp) {
    if (!imp) return NULL;
    int i = strtol(imp, NULL, 16);
    arm_id *aid = scan_arm_ids_file(i, ARM_ILLEGAL_PART);
    if (aid)
        return aid->implementer_str; /* g_free() will free all aid */
    return NULL;
}

gchar *arm_part(const gchar *imp, const gchar *part) {
    if (!imp || !part) return NULL;
    int i = strtol(imp, NULL, 16);
    int p = strtol(part, NULL, 16);
    arm_id *aid = scan_arm_ids_file(i, p);
    if (aid) {
        gchar *ret = g_strdup(aid->part_str);
        arm_id_free(aid);
        if (ret)
            return ret;
    }
    return NULL;
}

gchar *arm_decoded_name(const gchar *imp, const gchar *part, const gchar *var, const gchar *rev, const gchar *arch) {
    int i = 0, p = 0, v = 0, r = 0;
    const gchar *a = arm_arch(arch);
    gchar *istr = NULL, *pstr = NULL;

    i = strtol(imp, NULL, 16);
    p = strtol(part, NULL, 16);
    v = strtol(var, NULL, 16);
    r = strtol(rev, NULL, 16);

    arm_id *aid = scan_arm_ids_file(i, p);
    if (aid) {
        istr = aid->implementer_str; /* g_free(istr) will free all aid */
        pstr = g_strdup(aid->part_str);
    }
    if (!istr) istr = g_strdup_printf("impl:0x%02x", i);
    if (!pstr) pstr = g_strdup_printf("part:0x%03x", p);

    gchar *dnbuff = g_strdup_printf("%s %s r%dp%d (%s)", istr, pstr, v, r, a);

    g_free(istr);
    g_free(pstr);
    return dnbuff;
}

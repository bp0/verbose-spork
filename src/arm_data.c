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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

#include "arm_lscpu.c"

char *arm_decoded_name(const char *imp, const char *part, const char *var, const char *rev, const char *arch) {
    int i = 0, p = 0, v = 0, r = 0;
    int n = 0, m = 0;
    const char *a = arm_arch(arch);

    i = strtol(imp, NULL, 0);
    p = strtol(part, NULL, 0);
    v = strtol(var, NULL, 0);
    r = strtol(rev, NULL, 0);

    while(hw_implementer[n].id != -1) {
        if (hw_implementer[n].id == i)
            break;
        n++;
    }
    while(hw_implementer[n].parts[m].id != -1) {
        if (hw_implementer[n].parts[m].id == p)
            break;
        m++;
    }

    char *dnbuff = malloc(256);
    snprintf(dnbuff, 255, "%s %s r%dp%d (%s)",
        hw_implementer[n].name, hw_implementer[n].parts[m].name, v, r, a);
    return dnbuff;
}

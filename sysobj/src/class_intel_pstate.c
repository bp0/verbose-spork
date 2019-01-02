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

const gchar intel_pstate_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/cpu-freq/intel-pstate.txt")
    "\n";

static attr_tab pstate_items[] = {
    { "max_perf_pct", N_("limit the maximum P-State that will be requested by the driver"), OF_NONE, fmt_percent },
    { "min_perf_pct", N_("limit the minimum P-State that will be requested by the driver"), OF_NONE, fmt_percent },
    { "turbo_pct", N_("percentage of the total performance, supported by hardware, that is in the turbo range"), OF_NONE, fmt_percent },
    { "no_turbo", N_("turbo is disabled"), OF_NONE, fmt_1yes0no },
    { "num_pstates", N_("number of P-States that are supported by hardware") },
    ATTR_TAB_LAST
};

static sysobj_class cls_intel_pstate[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "intel_pstate", .pattern = "/sys/devices/system/cpu/intel_pstate", .flags = OF_CONST,
    .s_halp = intel_pstate_reference_markup_text },
  { SYSOBJ_CLASS_DEF
    .tag = "intel_pstate", .pattern = "/sys/devices/system/cpu/intel_pstate/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .attributes = pstate_items, .s_halp = intel_pstate_reference_markup_text },
};

void class_intel_pstate() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_intel_pstate); i++)
        class_add(&cls_intel_pstate[i]);
}

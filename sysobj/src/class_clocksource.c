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

/*
    [ARM] imx_timer1,OSTS,netx_timer,mpu_timer2,
            pxa_timer,timer3,32k_counter,timer0_1
    [AVR32] avr32
    [X86-32] vmi-timer;
            scx200_hrt on Geode; cyclone on IBM x440
    [MIPS] MIPS
    [PARISC] cr16
    [S390] tod
    [SH] SuperH
    [SPARC64] tick
*/

#include "sysobj.h"
#include "format_funcs.h"

static gchar *fmt_clocksource_type(sysobj *obj, int fmt_opts) {
    static lookup_tab clocksource_type[] = {
        /* x86 */
        { "jiffies", N_("the fallback clocksource, size of a jiffy is based on kernel constant HZ") },
        { "acpi_pm", N_("the ACPI Power Management Timer, fixed frequency of roughly 3.58 MHz") },
        { "tsc",     N_("the Time Stamp Counter, count of cycles since reset") },
        { "hpet",    N_("the High Precision Event Timer") },
        { "pit",     N_("a programmable interval timer") },
        /* arm */
        { "stc" },
        { "timer" }, // rpi1
        { "arch_sys_counter" }, //rpi2,3
        LOOKUP_TAB_LAST
    };
    gchar *ret = sysobj_format_lookup_tab(obj, clocksource_type, fmt_opts);
    if (ret) return ret;
    return simple_format(obj, fmt_opts);
}

static attr_tab clocksource_items[] = {
    { "available_clocksource" },
    { "current_clocksource", NULL, OF_NONE, fmt_clocksource_type },
    ATTR_TAB_LAST
};

static sysobj_class cls_clocksource[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "clocksource", .pattern = "/sys/devices/system/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem = "/sys/bus/clocksource", .v_lblnum = "clocksource",
    .s_node_format = "{{current_clocksource}}" },
  { SYSOBJ_CLASS_DEF
    .tag = "clocksource:attr", .pattern = "/sys/devices/system/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .v_subsystem_parent = "/sys/bus/clocksource", .v_lblnum_child = "clocksource",
    .attributes = clocksource_items },
};

void class_clocksource() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_clocksource); i++)
        class_add(&cls_clocksource[i]);
}

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

/*
 * - generators create virtual sysobj's
 *    - can't use sysobj_format, only sysobj_raw as there are no classes yet.
 * - classes provide interpretation and formatting of sysobj's
 */

void gen_sysobj();
void gen_dt();
void gen_pci_ids();
void gen_usb_ids();
void gen_arm_ids();
void gen_sdio_ids();
void gen_sdcard_ids();
void gen_dt_ids();
void gen_edid_ids();
void gen_os_release();
void gen_dmidecode();
void gen_rpi();
void gen_mobo(); /* requires :/extern/dmidecode :/raspberry_pi */
void gen_cpuinfo(); /* creates in :/cpu, before it exists */
void gen_meminfo();
void gen_procs(); /* requires :/cpu/cpuinfo */
void gen_gpu();   /* requires gen_*_ids, gen_dt */
void gen_storage();

void generators_init() {
    gen_sysobj(); /* internals, like vsysfs root (":") */

    gen_dt();
    gen_pci_ids();
    gen_usb_ids();
    gen_arm_ids();
    gen_sdio_ids();
    gen_sdcard_ids();
    gen_dt_ids();
    gen_edid_ids();
    gen_os_release();
    gen_dmidecode();
    gen_rpi();
    gen_mobo();
    gen_cpuinfo();
    gen_meminfo();
    gen_procs();
    gen_gpu();
    gen_storage();
}

void class_sysobj();
void class_lookup();
void class_power();
void class_os_release();
void class_rpi();
void class_mobo();
void class_cpuinfo();
void class_meminfo();
void class_procs();
void class_gpu();
void class_hwmon();
void class_devfreq();
void class_backlight();
void class_i2c();
void class_block();
void class_scsi();
void class_clocksource();
void class_mmc();
void class_media();
void class_intel_pstate();
void class_nvme();
void class_storage();
void class_aer();
void class_power_supply();

void class_uptime();
void class_dmi_id();
void class_dt();
void class_cpu();
void class_cpufreq();
void class_cpucache();
void class_cputopo();
void class_cpuidle();
void class_pci();
void class_usb();
void class_os_release();
void class_proc_alts();
void class_any_utf8();
void class_any_bus();
void class_extra();

void class_init() {
    generators_init();
    class_sysobj();
    class_lookup();

    class_power();
    class_proc_alts();

    class_os_release();
    class_rpi();
    class_mobo();
    class_cpuinfo();
    class_meminfo();
    class_procs();
    class_gpu();
    class_hwmon();
    class_devfreq();
    class_backlight();
    class_i2c();
    class_block();
    class_scsi();
    class_clocksource();
    class_mmc();
    class_media();
    class_intel_pstate();
    class_nvme();
    class_storage();
    class_aer();
    class_power_supply();

    class_cpu();
    class_cpufreq();
    class_cpucache();
    class_cputopo();
    class_cpuidle();
    class_pci();
    class_usb();
    class_uptime();

    class_extra();

/* consumes every direct child, careful with order */
    class_dmi_id();
    class_dt();

/* anything left that is human-readable */
    class_any_utf8(); /* OF_BLAST */

/* any unclaimed /sys/bus/* */
    class_any_bus(); /* OF_BLAST */
}

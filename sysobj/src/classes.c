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
/*
 * - generators create virtual sysobj's
 *    - can't use sysobj_format, only sysobj_raw as there are no classes yet.
 * - classes provide interpretation and formatting of sysobj's
 */

void gen_sysobj();
void gen_pci_ids();
void gen_usb_ids();
void gen_os_release();
void gen_dmidecode();
void gen_mobo(); /* requires :/dmidecode */
void gen_rpi();
void gen_cpuinfo();
void gen_meminfo();
void gen_procs(); /* requires :/cpuinfo */

void gen_computer(); /* formerly vo_computer */

void generators_init() {
    gen_sysobj(); /* internals, like vsysfs root (":"), and ":/watchlist" */

    gen_pci_ids();
    gen_usb_ids();
    gen_os_release();
    gen_dmidecode();
    gen_mobo();
    gen_rpi();
    gen_cpuinfo();
    gen_meminfo();
    gen_procs();

    //gen_computer();
}

void class_os_release();
void class_mobo();
void class_rpi();
void class_cpuinfo();
void class_meminfo();
void class_procs();

void class_uptime();
void class_dmi_id();
void class_dt();
void class_cpu();
void class_cpufreq();
void class_cpucache();
void class_cputopo();
void class_pci();
void class_usb();
void class_os_release();
void class_proc_alts();
void class_any_utf8();

void class_init() {
    generators_init();
    class_add_simple(":", _("Virtual sysfs root"), "vsfs", OF_NONE, 60);

    class_proc_alts();

    class_os_release();
    class_mobo();
    class_rpi();
    class_cpuinfo();
    class_meminfo();
    class_procs();

    class_cpu();
    class_cpufreq();
    class_cpucache();
    class_cputopo();
    class_pci();
    class_usb();
    class_os_release();
    class_uptime();
/* consumes every direct child, careful with order */
    class_dmi_id();
    class_dt();

/* anything left that is human-readable */
    class_any_utf8();
}

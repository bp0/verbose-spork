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

void vo_computer();

void class_dmi_id();
void class_dt();
void class_cpu();
void class_cpufreq();
void class_cpucache();
void class_cputopo();
void class_cpuinfo();
void class_pci();
void class_usb();
void class_os_release();
void class_any_utf8();

void class_init() {
    vo_computer();
    class_cpu();
    class_cpufreq();
    class_cpucache();
    class_cputopo();
    class_cpuinfo();
    class_pci();
    class_usb();
    class_os_release();
/* consumes every direct child, careful with order */
    class_dmi_id();
    class_dt();
/* anything left that is human-readable */
    class_any_utf8();
}

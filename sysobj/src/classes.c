
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

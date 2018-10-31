
#include "sysobj.h"

void vo_computer();

void class_dmi_id();
void class_cpu();
void class_cpufreq();
void class_cpucache();
void class_cputopo();
void class_any_utf8();

void class_init() {
    vo_computer();

    class_cpu();
    class_cpufreq();
    class_cpucache();
    class_cputopo();
/* consumes every direct child, careful with order */
    class_dmi_id();
/* anything left that is human-readable */
    class_any_utf8();
}

void class_cleanup() {
    class_free_list();
}


#include "sysobj.h"

void class_dmi_id();
void class_cpu();
void class_cpufreq();
void class_cpucache();
//void class_cputopo();
void class_any_utf8();

void class_init() {
    class_cpu();
    class_cpufreq();
    class_cpucache();
    //class_cputopo();

/* consumes every direct child, careful with order */
    class_dmi_id();
/* consumes every child, careful with order */
    class_any_utf8();
}

void class_cleanup() {
    class_free_list();
}

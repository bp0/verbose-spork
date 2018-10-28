
#include "sysobj.h"

void class_dmi_id();
void class_cpu();
void class_cpufreq();
void class_cpucache();
//void class_cputopo();

void class_init() {
    class_dmi_id();
    class_cpu();
    class_cpufreq();
    class_cpucache();
    //class_cputopo();
}

void class_cleanup() {
    class_free_list();
}

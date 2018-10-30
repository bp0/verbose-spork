
#include "sysobj.h"

typedef struct {
    gchar *tag;
    gchar *label; /* not yet translated */
    gchar *info;  /* not yet translated */
} cpufreq_item;

static const cpufreq_item cpufreq_items[] = {
    { "scaling_min_freq", N_("Minimum"), N_("Minimum clock frequency (via scaling driver)") },
    { "scaling_max_freq", N_("Maximum"), N_("Maximum clock frequency (via scaling driver)") },
    { "scaling_cur_freq", N_("Current"), N_("Current clock frequency (via scaling driver)") },

    { "bios_limit", N_("Maximum"), N_("Maximum clock frequency (via BIOS)") },

    { "cpuinfo_min_freq", N_("Minimum"), N_("Minimum clock frequency (via cpuinfo)") },
    { "cpuinfo_max_freq", N_("Maximum"), N_("Maximum clock frequency (via cpuinfo)") },
    { "cpuinfo_cur_freq", N_("Current"), N_("Current clock frequency (via cpuinfo)") },

    { "cpuinfo_transition_latency", N_("Transition Latency"), NULL },

    { "freqdomain_cpus", NULL, N_("Logical CPUs that share the same base clock") },
    { "affected_cpus",   NULL, N_("Logical CPUs affected by change to scaling_setspeed") },
    { "related_cpus",    NULL, N_("Related logical CPUs") },
    { NULL, NULL, NULL }
};

int cpufreq_item_find(gchar *tag) {
    int i = 0;
    while(cpufreq_items[i].tag) {
        if ( strcmp(cpufreq_items[i].tag, tag) == 0 )
            return i;
        i++;
    }
    return -1;
}

gboolean cpufreq_verify(sysobj *obj) {
    /* child of "policy%d" or "cpufreq" */
    if (obj) {
        gboolean verified =
            verify_lblnum_child(obj, "policy")
            | verify_parent_name(obj, "cpufreq");
        if (verified && cpufreq_item_find(obj->name) != -1)
            return TRUE;
    }
    return FALSE;
}

gchar *cpufreq_format_ns(sysobj *obj, int fmt_opts) {
    if (obj) {
        if (obj->data.was_read && obj->data.str) {
            uint32_t ns = strtoul(obj->data.str, NULL, 10);
            if (fmt_opts & FMT_OPT_NO_UNIT)
                return g_strdup_printf("%u", ns);
            else
                return g_strdup_printf("%u %s", ns, _("ns"));
        }
    }
    return simple_format(obj, fmt_opts);
}

gchar *cpufreq_format_khz(sysobj *obj, int fmt_opts) {
    if (obj) {
        if (obj->data.was_read && obj->data.str) {
            uint32_t khz = strtoul(obj->data.str, NULL, 10);
            double mhz = (double)khz / 1000;
            if (fmt_opts & FMT_OPT_NO_UNIT)
                return g_strdup_printf("%.3f", mhz);
            else
                return g_strdup_printf("%.3f %s", mhz, _("MHz"));
        }
    }
    return simple_format(obj, fmt_opts);
}

const gchar *cpufreq_label(sysobj *obj) {
    if (obj) {
        int i = cpufreq_item_find(obj->name);
        return _(cpufreq_items[i].label);
    }
    return NULL;
}

const gchar *cpufreq_info(sysobj *obj) {
    if (obj) {
        int i = cpufreq_item_find(obj->name);
        return _(cpufreq_items[i].info);
    }
    return NULL;
}

double cpufreq_update_interval_for_khz(sysobj *obj) {
    if (obj)
        return 100;
    return 0;
}

static void cpufreq_class_for_hz(sysobj_class *c) {
    c->f_verify = cpufreq_verify;
    c->f_format = cpufreq_format_khz;
    c->f_label = cpufreq_label;
    c->f_info = cpufreq_info;
    c->f_update_interval = cpufreq_update_interval_for_khz;
    c->f_compare = compare_str_base10;
}

static void cpufreq_class_for_ns(sysobj_class *c) {
    c->f_verify = cpufreq_verify;
    c->f_format = cpufreq_format_ns;
    c->f_label = cpufreq_label;
    c->f_info = cpufreq_info;
    c->f_update_interval = NULL;
    c->f_compare = compare_str_base10;
}

gboolean cpufreq_verify_policy(sysobj *obj) {
    return verify_lblnum(obj, "policy");
}

gchar *cpufreq_format_policy(sysobj *obj, int fmt_opts) {
    if (obj) {
        gchar *ret = NULL;
        /* get nice description */
        gchar *scl_min =
            sysobj_format_from_fn(obj->path, "scaling_min_freq", fmt_opts | FMT_OPT_PART | FMT_OPT_NO_UNIT );
        gchar *scl_max =
            sysobj_format_from_fn(obj->path, "scaling_max_freq", fmt_opts | FMT_OPT_PART);
        gchar *scl_gov =
            sysobj_format_from_fn(obj->path, "scaling_governor", fmt_opts | FMT_OPT_PART);
        gchar *scl_drv =
            sysobj_format_from_fn(obj->path, "scaling_driver", fmt_opts | FMT_OPT_PART);
        gchar *scl_cur =
            sysobj_format_from_fn(obj->path, "scaling_cur_freq", fmt_opts | FMT_OPT_PART);

        ret = g_strdup_printf("%s %s (%s - %s) [ %s ]",
                scl_drv, scl_gov, scl_min, scl_max, scl_cur );

        g_free(scl_min);
        g_free(scl_max);
        g_free(scl_gov);
        g_free(scl_drv);
        g_free(scl_cur);
        return ret;
    }
    return simple_format(obj, fmt_opts);
}

void class_cpufreq() {
    sysobj_class *c = NULL;

    c = g_new0(sysobj_class, 1);
    c->pattern = "*/cpufreq/policy*";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpufreq_verify_policy;
    c->s_label = _("Frequency Scaling Policy");
    c->f_format = cpufreq_format_policy;
    c->f_update_interval = cpufreq_update_interval_for_khz;
    class_add(c);

#define CLS_PLZ(pat, unit)       \
    c = g_new0(sysobj_class, 1); \
    c->tag = "cpufreq";          \
    c->pattern = pat;            \
    c->flags = OF_GLOB_PATTERN;  \
    cpufreq_class_for_##unit (c); \
    class_add(c);

    CLS_PLZ("*/cpufreq/policy*/scaling_???_freq", hz);
    CLS_PLZ("*/cpufreq/policy*/cpuinfo_???_freq", hz);
    CLS_PLZ("*/cpufreq/policy*/cpuinfo_transition_latency", ns);
    CLS_PLZ("*/cpufreq/policy*/bios_limit", hz);

}

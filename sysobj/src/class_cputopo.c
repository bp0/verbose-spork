
#include "sysobj.h"

#include "cpubits.c"

static sysobj_class *cls_cputopo;

static const struct { gchar *rp; gchar *lbl; int extra_flags; } cputopo_items[] = {
    { "topology",  N_("CPU Topology Information"), OF_NONE },
    { "physical_package_id", N_("Processor/Socket"), OF_NONE },
    { "core_id",             N_("CPU Core"), OF_NONE }, //TODO: of package?
    { NULL, NULL, 0 }
};

int cputopo_lookup(const gchar *key) {
    int i = 0;
    while(cputopo_items[i].rp) {
        if (strcmp(key, cputopo_items[i].rp) == 0)
            return i;
        i++;
    }
    return -1;
}

gboolean cputopo_verify(sysobj *obj) {
    int i = cputopo_lookup(obj->name);
    if (i != -1)
        return TRUE;
    if (!strcmp(obj->name, "topology")) {
        if (verify_lblnum_child(obj, "cpu"))
            return TRUE;
    }
/*
     else if (verify_parent_name(obj, "topology"))
        return TRUE;
*/
    return FALSE;
}

const gchar *cputopo_label(sysobj *obj) {
    int i = cputopo_lookup(obj->name);
    if (i != -1)
        return _(cputopo_items[i].lbl);
    return NULL;
}

void cpu_pct(sysobj *obj, int *logical, int *pack, int *core_of_pack, int *thread_of_core) {
    gchar *pn = sysobj_parent_name(obj);
    *logical = util_get_did(pn, "cpu");

    gchar *pack_str =
        sysobj_format_from_fn(obj->path, "physical_package_id", FMT_OPT_RAW_OR_NULL );
    gchar *core_id =
        sysobj_format_from_fn(obj->path, "core_id", FMT_OPT_RAW_OR_NULL );
    gchar *thread_sibs =
        sysobj_format_from_fn(obj->path, "thread_siblings_list", FMT_OPT_RAW_OR_NULL );

    *pack = pack_str ? strtol(pack_str, NULL, 10) : -1;
    *core_of_pack = core_id ? strtol(core_id, NULL, 10) : -1;
    *thread_of_core = -1;
    if (thread_sibs) {
        cpubits *bits = cpubits_from_str(thread_sibs);
        int sib = cpubits_min(bits);
        int i = 0;
        while (sib != *logical) {
            sib = cpubits_next(bits, sib, -1);
            i++;
        }
        if (sib == *logical)
            *thread_of_core = i;
        free(bits);
    }

    g_free(pack_str);
    g_free(core_id);
    g_free(thread_sibs);
    g_free(pn);
}

gchar *topology_summary(sysobj *obj, int fmt_opts) {
    static const char sfmt[] = N_("(%d){P%d:C%d:T%d}");
    static const char lfmt[] = N_("Logical CPU %d {Processor %d : Core %d : Thread %d}");
    const char *fmt = (fmt_opts & FMT_OPT_SHORT) ? sfmt : lfmt;
    int logical = 0, pack = 0, core_of_pack = 0, thread_of_core = 0;
    cpu_pct(obj, &logical, &pack, &core_of_pack, &thread_of_core);
    return g_strdup_printf(
        (fmt_opts & FMT_OPT_NO_TRANSLATE) ? fmt : _(fmt),
        logical, pack, core_of_pack, thread_of_core);
}

gchar *cputopo_format(sysobj *obj, int fmt_opts) {
    if (!strcmp(obj->name, "topology"))
        return topology_summary(obj, fmt_opts);
    return simple_format(obj, fmt_opts);
}

/* remember to handle obj == NULL */
guint cputopo_flags(sysobj *obj) {
    if (obj) {
        int i = cputopo_lookup(obj->name);
        if (i != -1)
            return obj->cls->flags | cputopo_items[i].extra_flags;
    }
    return cls_cputopo ? cls_cputopo->flags : OF_NONE;
}

void class_cputopo() {
    cls_cputopo = class_new();
    cls_cputopo->tag = "cputopo";
    cls_cputopo->pattern = "*/cpu*/topology*";
    cls_cputopo->flags = OF_GLOB_PATTERN;
    cls_cputopo->f_verify = cputopo_verify;
    cls_cputopo->f_label = cputopo_label;
    cls_cputopo->f_format = cputopo_format;
    cls_cputopo->f_flags = cputopo_flags;
    class_add(cls_cputopo);
}


#include <stdio.h> /* for sscanf() */
#include "sysobj.h"

gboolean cpu_verify(sysobj *obj) {
    return verify_lblnum(obj, "cpu");
}

gboolean cpu_verify_child(sysobj *obj) {
    return verify_lblnum_child(obj, "cpu");
}

uint32_t cpu_get_id(const gchar *name) {
    uint32_t id = 0;
    sscanf(name, "cpu%u", &id);
    return id;
}

gchar *cpu_format(sysobj *obj, int fmt_opts) {
    if (obj) {
        /* TODO: get nice description */
        return g_strdup_printf(_("Logical CPU #%u"), cpu_get_id(obj->name) );
    }
    return simple_format(obj, fmt_opts);
}

gchar *cpu_format_1yes0no(sysobj *obj, int fmt_opts) {
    if (obj && obj->data.str) {
        int value = strtol(obj->data.str, NULL, 10);
        return g_strdup_printf("[%d] %s", value, value ? _("Yes") : _("No") );
    }
    return simple_format(obj, fmt_opts);
}

void class_cpu() {
    sysobj_class *c = NULL;

    c = g_new0(sysobj_class, 1);
    c->pattern = "*/cpu*";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpu_verify;
    c->f_format = cpu_format;
    c->s_label = _("Logical CPU");
    c->s_info = NULL; //TODO:
    class_add(c);

    c = g_new0(sysobj_class, 1);
    c->pattern = "*/cpu*/online";
    c->flags = OF_GLOB_PATTERN;
    c->f_verify = cpu_verify_child;
    c->f_format = cpu_format_1yes0no;
    c->s_label = _("Is Online");
    c->s_info = NULL; //TODO:
    class_add(c);

    class_add_simple("microcode/version", _("Version"), OF_REQ_ROOT);
}

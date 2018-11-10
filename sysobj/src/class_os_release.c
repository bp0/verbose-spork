
#include "sysobj.h"

#define PATH_TO_OS_RELEASE "/usr/lib/os-release"

gchar* os_release_log = NULL;

static gchar *os_release_feature_format(sysobj *obj, int fmt_opts);
static gchar *os_release_format(sysobj *obj, int fmt_opts);
static guint os_release_flags(sysobj *obj);
static gchar *os_release_messages(const gchar *path);
static double os_release_update_interval(sysobj *obj);

void class_os_release_cleanup();

#define CLS_OS_RELEASE_FLAGS OF_GLOB_PATTERN | OF_CONST

static sysobj_class cls_os_release[] = {
  /* all else */
  { .tag = "os_release", .pattern = ":os_release", .flags = CLS_OS_RELEASE_FLAGS,
    .f_format = os_release_format, .f_flags = os_release_flags, .f_update_interval = os_release_update_interval, .f_cleanup = class_os_release_cleanup },
  { .tag = "os_release", .pattern = ":os_release/*", .flags = CLS_OS_RELEASE_FLAGS,
    .f_format = os_release_format, .f_flags = os_release_flags, .f_update_interval = os_release_update_interval },
};

static sysobj_virt vol[] = {
    { .path = ":os_release/_messages", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = os_release_messages, .f_get_type = NULL },
    { .path = ":os_release", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
};

void os_release_msg(char *fmt, ...) {
    gchar *buf, *tmp;
    va_list args;

    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);

    tmp = g_strdup_printf("%s%s\n", os_release_log, buf);
    g_free(os_release_log);
    os_release_log = tmp;
}

static gchar *os_release_messages(const gchar *path) {
    PARAM_NOT_UNUSED(path);
    return g_strdup(os_release_log ? os_release_log : "");
}

static gchar *os_release_format(sysobj *obj, int fmt_opts) {
    if (!strcmp(":os_release", obj->name)) {
        gchar *name = sysobj_raw_from_fn(":os_release", "NAME");
        gchar *version = sysobj_raw_from_fn(":os_release", "VERSION");
        util_strstrip_double_quotes_dumb(name);
        util_strstrip_double_quotes_dumb(version);
        gchar *full_name = g_strdup_printf("%s %s", name, version);
        g_free(name);
        g_free(version);
        return full_name;
    }
    return simple_format(obj, fmt_opts);
}

static guint os_release_flags(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return CLS_OS_RELEASE_FLAGS;
}

static double os_release_update_interval(sysobj *obj) {
    PARAM_NOT_UNUSED(obj);
    return 60.0; /* there could be an upgrade */
}
void os_release_scan() {
    sysobj *obj = sysobj_new_from_fn(PATH_TO_OS_RELEASE, NULL);
    if (obj->exists) {
        sysobj_read_data(obj);
        sysobj_virt_from_kv(":os_release", obj->data.str);
    }
    sysobj_free(obj);
}

void class_os_release_cleanup() {
}

void class_os_release() {
    int i = 0;

    os_release_log = g_strdup("");

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_os_release); i++) {
        class_add(&cls_os_release[i]);
    }

    if (!sysobj_exists_from_fn(PATH_TO_OS_RELEASE, NULL)) {
        os_release_msg("os_release not found at %s", PATH_TO_OS_RELEASE);
    }

    os_release_scan();
}

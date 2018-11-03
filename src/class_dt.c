
#include "sysobj.h"

#define DTROOT "/sys/firmware/devicetree/base"

gchar *dt_messages(const gchar *path) {
    return g_strdup(path);
}

static sysobj_virt vol[] = {
    { .path = ":devicetree", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":devicetree/base", .str = DTROOT,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":devicetree/messages", .str = "*",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = dt_messages, .f_get_type = NULL },
};

void class_dt() {
    int i = 0;
    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }
}


#include "sysobj.h"

static sysobj_class *cls_any_utf8;

gboolean any_utf8_verify(sysobj *obj) {
    gboolean was_read = obj->data.was_read;
    gboolean verified = FALSE;
    if (!was_read)
        sysobj_read_data(obj);
    if (obj->data.is_utf8 && obj->data.len)
        verified = TRUE;
    if (!was_read)
        sysobj_unread_data(obj);
    return verified;
}

void class_any_utf8() {
    cls_any_utf8 = class_new();
    cls_any_utf8->tag = "any";
    cls_any_utf8->pattern = "*";
    cls_any_utf8->flags = OF_GLOB_PATTERN;
    cls_any_utf8->f_verify = any_utf8_verify;
    class_add(cls_any_utf8);
}

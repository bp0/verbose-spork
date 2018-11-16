#ifndef BP_PIN_INSPECT_H
#define BP_PIN_INSPECT_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "uri_handler.h"
#include "sysobj.h"
#include "pin.h"

G_BEGIN_DECLS

#define BP_PIN_INSPECT_TYPE            (bp_pin_inspect_get_type())
#define BP_PIN_INSPECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BP_PIN_INSPECT_TYPE, bpPinInspect))
#define BP_PIN_INSPECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BP_PIN_INSPECT_TYPE, bpPinInspectClass))
#define IS_BP_PIN_INSPECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BP_PIN_INSPECT_TYPE))
#define IS_BP_PIN_INSPECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BP_PIN_INSPECT_TYPE))

typedef struct _bpPinInspect       bpPinInspect;
typedef struct _bpPinInspectClass  bpPinInspectClass;

struct _bpPinInspect {
    GtkPaned parent_instance;
};

struct _bpPinInspectClass {
    GtkPanedClass parent_class;
};

GType bp_pin_inspect_get_type(void) G_GNUC_CONST;
GtkWidget *bp_pin_inspect_new(void);
void bp_pin_inspect_do(bpPinInspect *s, const pin *p, int fmt_opts);
const pin *bp_pin_inspect_get_pin(bpPinInspect *s);

G_END_DECLS

#endif /* BP_PIN_INSPECT_H */

#ifndef BP_SYSOBJ_VIEW_H
#define BP_SYSOBJ_VIEW_H

/* signals *
 * landed:          void landed(gpointer user_data);
 * item-activated:  void activate(gpointer user_data, const gchar *sysobj_path);
 * item-selected:   void activate(gpointer user_data, guint pin_index);
*/

#include <glib-object.h>
#include <gtk/gtk.h>
#include "pin.h"

G_BEGIN_DECLS

#define BP_SYSOBJ_VIEW_TYPE            (bp_sysobj_view_get_type())
#define BP_SYSOBJ_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BP_SYSOBJ_VIEW_TYPE, bpSysObjView))
#define BP_SYSOBJ_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BP_SYSOBJ_VIEW_TYPE, bpSysObjViewClass))
#define IS_BP_SYSOBJ_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BP_SYSOBJ_VIEW_TYPE))
#define IS_BP_SYSOBJ_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BP_SYSOBJ_VIEW_TYPE))

typedef struct _bpSysObjView       bpSysObjView;
typedef struct _bpSysObjViewClass  bpSysObjViewClass;

struct _bpSysObjView {
    GtkPaned parent_instance;
};

struct _bpSysObjViewClass {
    GtkPanedClass parent_class;
};

GType bp_sysobj_view_get_type(void) G_GNUC_CONST;
GtkWidget *bp_sysobj_view_new(void);

void bp_sysobj_view_set_path(bpSysObjView *s, const gchar *new_path);
void bp_sysobj_view_set_fmt_opts(bpSysObjView *s, int fmt_opts);  /* default: OF_NONE */
void bp_sysobj_view_set_max_depth(bpSysObjView *s, int fmt_opts);  /* default: 1 */
void bp_sysobj_view_set_include_target(bpSysObjView *s, gboolean include_target);
void bp_sysobj_view_show_inspector(bpSysObjView *s);
void bp_sysobj_view_hide_inspector(bpSysObjView *s);
gboolean bp_sysobj_view_inspector_is_visible(bpSysObjView *s);
gboolean bp_sysobj_view_refresh(bpSysObjView *s);

const pin *bp_sysobj_view_get_selected_pin(bpSysObjView *s);

G_END_DECLS

#endif /* BP_SYSOBJ_VIEW_H */

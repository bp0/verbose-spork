#ifndef BP_SYSOBJ_BROWSER_H
#define BP_SYSOBJ_BROWSER_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BP_SYSOBJ_BROWSER_TYPE            (bp_sysobj_browser_get_type())
#define BP_SYSOBJ_BROWSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BP_SYSOBJ_BROWSER_TYPE, bpSysObjBrowser))
#define BP_SYSOBJ_BROWSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BP_SYSOBJ_BROWSER_TYPE, bpSysObjBrowserClass))
#define IS_BP_SYSOBJ_BROWSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BP_SYSOBJ_BROWSER_TYPE))
#define IS_BP_SYSOBJ_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BP_SYSOBJ_BROWSER_TYPE))

typedef struct _bpSysObjBrowser       bpSysObjBrowser;
typedef struct _bpSysObjBrowserClass  bpSysObjBrowserClass;

struct _bpSysObjBrowser {
    GtkBox parent_instance;
};

struct _bpSysObjBrowserClass {
    GtkBoxClass parent_class;
};

GType bp_sysobj_browser_get_type(void) G_GNUC_CONST;
GtkWidget *bp_sysobj_browser_new(void);

void bp_sysobj_browser_navigate(bpSysObjBrowser *s, const gchar *new_location);

G_END_DECLS

#endif /* BP_SYSOBJ_BROWSER_H */

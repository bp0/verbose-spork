#ifndef BP_SYSOBJ_SEARCH_H
#define BP_SYSOBJ_SEARCH_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BP_SYSOBJ_SEARCH_TYPE            (bp_sysobj_search_get_type())
#define BP_SYSOBJ_SEARCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BP_SYSOBJ_SEARCH_TYPE, bpSysObjSearch))
#define BP_SYSOBJ_SEARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BP_SYSOBJ_SEARCH_TYPE, bpSysObjSearchClass))
#define IS_BP_SYSOBJ_SEARCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BP_SYSOBJ_SEARCH_TYPE))
#define IS_BP_SYSOBJ_SEARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BP_SYSOBJ_SEARCH_TYPE))

typedef struct _bpSysObjSearch       bpSysObjSearch;
typedef struct _bpSysObjSearchClass  bpSysObjSearchClass;

struct _bpSysObjSearch {
    GtkBox parent_instance;
};

struct _bpSysObjSearchClass {
    GtkBoxClass parent_class;
};

GType bp_sysobj_search_get_type(void) G_GNUC_CONST;
GtkWidget *bp_sysobj_search_new(void);

G_END_DECLS

#endif /* BP_SYSOBJ_SEARCH_H */

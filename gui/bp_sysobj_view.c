
#include "bp_sysobj_view.h"
#include "bp_pin_inspect.h"
#include "sysobj.h"
#include "pin.h"

#define UPDATE_TIMER_SECONDS 0.2
#define UPDATE_LIST_TIMER_SECONDS 4

enum {
    SIG_LANDED,
    SIG_ITEM_SELECTED,
    SIG_ITEM_ACTIVATED,
    SIG_LAST,
};
static guint _signals[SIG_LAST] = { 0 };

/* Forward declarations */
static void _create(bpSysObjView *s);
static void _cleanup(bpSysObjView *s);
static gboolean _update_store(bpSysObjView *s);

static void _expand_all(bpSysObjView *s);
static void _select_first_item(bpSysObjView *s);

/* ... captured signals */
void _row_changed(GtkTreeView *tree_view, gpointer user_data);
void _row_activate(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

static void __pins_list_view_fill(bpSysObjView *s);

/* Private class member */
#define BP_SYSOBJ_VIEW_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    BP_SYSOBJ_VIEW_TYPE, bpSysObjViewPrivate))

typedef struct _bpSysObjViewPrivate bpSysObjViewPrivate;

struct _bpSysObjViewPrivate {
    GtkTreeStore *store;
    GtkWidget *view;
    gchar *new_target;
    sysobj *obj;
    int fmt_opts;
    pin_list *pins;
    gboolean include_target;
    int max_depth;
    gboolean show_inspector;
    GtkWidget *pi;

    gint refresh_timer_timeout_id;
    gint refresh_timer_interval_ms;
    gint listing_timer_timeout_id;
    gint listing_timer_interval_ms;
};

G_DEFINE_TYPE(bpSysObjView, bp_sysobj_view, GTK_TYPE_PANED);

/* Initialize the bpSysObjView class */
static void
bp_sysobj_view_class_init(bpSysObjViewClass *klass)
{
    /* TODO: g_type_class_add_private deprecated in glib 2.58 */

    /* Add private indirection member */
    g_type_class_add_private(klass, sizeof(bpSysObjViewPrivate));

    /* Signals */
    _signals[SIG_LANDED] = g_signal_newv ("landed",
                 G_TYPE_OBJECT,
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                 NULL /* closure */,
                 NULL /* accumulator */,
                 NULL /* accumulator data */,
                 NULL /* C marshaller */,
                 G_TYPE_NONE /* return_type */,
                 0     /* n_params */,
                 NULL  /* param_types */);

    _signals[SIG_ITEM_ACTIVATED] = g_signal_new ("item-activated",
                 G_TYPE_OBJECT,
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                 0 /* class_offset */,
                 NULL /* accumulator */,
                 NULL /* accumulator data */,
                 NULL /* C marshaller */,
                 G_TYPE_NONE /* return_type */,
                 1     /* n_params */,
                 G_TYPE_POINTER);

    _signals[SIG_ITEM_SELECTED] = g_signal_new ("item-selected",
                 G_TYPE_OBJECT,
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                 0 /* class_offset */,
                 NULL /* accumulator */,
                 NULL /* accumulator data */,
                 NULL /* C marshaller */,
                 G_TYPE_NONE /* return_type */,
                 1     /* n_params */,
                 G_TYPE_UINT);

}

/* Initialize a new bpSysObjView instance */
static void
bp_sysobj_view_init(bpSysObjView *s)
{
    /* This means that bpSysObjView doesn't supply its own GdkWindow */
    gtk_widget_set_has_window(GTK_WIDGET(s), FALSE);
    /* Set redraw on allocate to FALSE if the top left corner of your widget
     * doesn't change when it's resized; this saves time */
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(s), FALSE);

    /* Initialize private members */
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    memset(priv, sizeof(bpSysObjViewPrivate), 0);

    priv->show_inspector = TRUE;
    priv->include_target = TRUE;
    priv->max_depth = 1;

    priv->refresh_timer_interval_ms = (UPDATE_TIMER_SECONDS * 1000.0);
    priv->refresh_timer_timeout_id = g_timeout_add(priv->refresh_timer_interval_ms, (GSourceFunc)bp_sysobj_view_refresh, s);

    //priv->listing_timer_interval_ms = (UPDATE_LIST_TIMER_SECONDS * 1000.0);
    //priv->listing_timer_timeout_id = g_timeout_add(priv->listing_timer_interval_ms, (GSourceFunc)_update_store, s);

    _create(s);

    g_signal_connect(s, "destroy", G_CALLBACK(_cleanup), NULL);
}

/* Return a new bpSysObjView cast to a GtkWidget */
GtkWidget *
bp_sysobj_view_new()
{
    //g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, NULL);
    return GTK_WIDGET(g_object_new(bp_sysobj_view_get_type(), NULL));
}

/* ------------------------------ */

enum
{
   KV_COL_ICON,
   KV_COL_KEY,
   KV_COL_LABEL,
   KV_COL_VALUE,
   KV_COL_TAG,
   KV_COL_INDEX,
   KV_COL_LIVE,
   KV_N_COLUMNS
};

static const char *kv_col_names[] = {
    "", /* icon */
    "Item",
    "Label",
    "Value",
    "Tag",
    "Index",
    "Is Live",
};

static void _cleanup(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    g_source_remove(priv->refresh_timer_timeout_id);
    //g_source_remove(priv->listing_timer_timeout_id);
    pins_free(priv->pins);
    sysobj_free(priv->obj);
    g_free(priv->new_target);
}

static void _create(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);

    /* tree store and tree view */
    priv->store = gtk_tree_store_new(KV_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    priv->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(priv->store));

    GtkWidget *list_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(list_scroll), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(list_scroll), priv->view);
    gtk_widget_show(priv->view);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* only icon, name, and value */
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes
        (kv_col_names[KV_COL_ICON], renderer, "icon-name", KV_COL_ICON, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes
        (kv_col_names[KV_COL_KEY], renderer, "text", KV_COL_KEY, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes
        (kv_col_names[KV_COL_VALUE], renderer, "markup", KV_COL_VALUE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

    /* pin inspector */
    priv->pi = bp_pin_inspect_new();

    /* panes */
    gtk_orientable_set_orientation(GTK_ORIENTABLE(s), GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(s), 420);
    gtk_paned_pack1(GTK_PANED(s), list_scroll, TRUE, FALSE); gtk_widget_show(list_scroll);
    gtk_paned_pack2(GTK_PANED(s), priv->pi, FALSE, FALSE); gtk_widget_show(priv->pi);

    /* fmt_opts */
    priv->fmt_opts = FMT_OPT_NONE;

    /* pin list */
    priv->pins = pins_new();

    /* signal capture */
    g_signal_connect(priv->view, "cursor-changed", G_CALLBACK(_row_changed), s);
    g_signal_connect(priv->view, "row-activated", G_CALLBACK(_row_activate), s);
}

void _row_changed(GtkTreeView *tree_view, gpointer user_data) {
    bpSysObjView *s = (bpSysObjView *)user_data;
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    GtkTreeIter iter;
    GtkTreePath *path;

    int pi = -1;
    gtk_tree_view_get_cursor(tree_view, &path, NULL);
    if (path) {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter, KV_COL_INDEX, &pi, -1);
    }
    if (priv->show_inspector) {
        if (pi >= 0) {
            pin *p = pins_get_nth(priv->pins, pi);
            bp_pin_inspect_do(BP_PIN_INSPECT(priv->pi), p, priv->fmt_opts);
        } else
            bp_pin_inspect_do(BP_PIN_INSPECT(priv->pi), NULL, 0);
    }
    g_signal_emit(s, _signals[SIG_ITEM_SELECTED], 0, pi);
}

void _row_activate(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    bpSysObjView *s = (bpSysObjView *)user_data;
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    GtkTreeIter iter;
    int pi = -1;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->store), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter, KV_COL_INDEX, &pi, -1);
    if (pi >= 0) {
        pin *p = pins_get_nth(priv->pins, pi);
        g_signal_emit(s, _signals[SIG_ITEM_ACTIVATED], 0, g_strdup(p->obj->path_req));
    }
}

static gboolean _update_store(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    gboolean is_new = FALSE;

    if (priv->new_target) {
        is_new = TRUE;
        /* clear the old */
        gtk_tree_store_clear(priv->store);
        pins_clear(priv->pins);
        if (priv->obj)
            sysobj_free(priv->obj);
        /* load the new */
        priv->obj = sysobj_new_from_fn(priv->new_target, NULL);
        g_free(priv->new_target);
        priv->new_target = NULL;
    }

    if (!priv->obj) goto _update_store_finish;

    /* load/update store */
    //TODO: what if a symlink target changes?
    sysobj_fscheck(priv->obj);
    sysobj_read_data(priv->obj);

    if (priv->include_target)
        pins_add_from_fn(priv->pins, priv->obj->path_req, NULL);

    if (priv->obj->exists) {
        gchar *fn = NULL;
        GSList *childs = sysobj_children(priv->obj, NULL, NULL, TRUE);
        for(GSList *l = childs; l; l = l->next) {
            fn = (gchar*)l->data;
            pins_add_from_fn(priv->pins, priv->obj->path_req, fn);
        }
        g_slist_free_full(childs, g_free);
    }

    if (is_new) {
        __pins_list_view_fill(s);
        _expand_all(s);
        _select_first_item(s);
        g_signal_emit(s, _signals[SIG_LANDED], 0);
    }

    /* will be refreshed in 0.2 seconds */

_update_store_finish:
    return G_SOURCE_REMOVE; /* remove from the main loop */
}

gboolean __pins_list_view_update_row(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, bpSysObjViewPrivate *priv) {
    int pi, live;
    const pin *p = NULL;
    gtk_tree_model_get(model, iter, KV_COL_LIVE, &live, -1);
    if (live) {
        gtk_tree_model_get(model, iter, KV_COL_INDEX, &pi, -1);
        p = pins_pin_if_updated_since(priv->pins, pi, UPDATE_TIMER_SECONDS);
        if (p) {
            gchar *nice = sysobj_format(p->obj, priv->fmt_opts | FMT_OPT_LIST_ITEM);
            gtk_tree_store_set(GTK_TREE_STORE(model), iter, KV_COL_VALUE, nice, -1);
            g_free(nice);
            if (p == bp_pin_inspect_get_pin(BP_PIN_INSPECT(priv->pi)) ) {
                bp_pin_inspect_do(BP_PIN_INSPECT(priv->pi), p, priv->fmt_opts);
            }
        }
    }
    return FALSE;
}

gboolean bp_sysobj_view_refresh(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    pins_refresh(priv->pins);
    gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store), (GtkTreeModelForeachFunc)__pins_list_view_update_row, priv);
    return G_SOURCE_CONTINUE;
}

void _expand_all(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(priv->view));
}

void _select_first_item(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    GtkTreePath *path = gtk_tree_path_new_from_string("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW(priv->view),
                path, NULL, FALSE);
}

void bp_sysobj_view_set_max_depth(bpSysObjView *s, int max_depth) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    priv->max_depth = max_depth;
}

void bp_sysobj_view_set_fmt_opts(bpSysObjView *s, int fmt_opts) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    priv->fmt_opts = fmt_opts;
}

void bp_sysobj_view_set_path(bpSysObjView *s, const gchar *new_path) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    if (priv->new_target)
        g_free(priv->new_target); /* I guess it's new new target */
    priv->new_target = g_strdup(new_path);
    g_idle_add((GSourceFunc)_update_store, s);
}

static void __pins_list_view_fill(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    GtkTreeIter iter, parent;

    gtk_tree_store_clear(priv->store);

    int i = 0;
    GSList *l = priv->pins->list;
    while (l) {
        pin *p = l->data;

        gchar *label = g_strdup(sysobj_label(p->obj));
        gchar *nice = sysobj_format(p->obj, priv->fmt_opts | FMT_OPT_LIST_ITEM);
        gchar *tag = g_strdup_printf("{%s}", p->obj->cls ? (p->obj->cls->tag ? p->obj->cls->tag : p->obj->cls->pattern) : "none");
        gboolean is_live = !(p->update_interval == UPDATE_INTERVAL_NEVER);

        gchar *nice_key = (0 && label)
            ? g_strdup_printf("%s <small>(%s)</small>", label, p->obj->name_req)
            : g_strdup(p->obj->name_req);

        if (i == 0)
            gtk_tree_store_append(priv->store, &parent, NULL);
        else
            gtk_tree_store_append(priv->store, &iter, &parent);

        gtk_tree_store_set(priv->store, i ? &iter : &parent,
                    KV_COL_ICON, (p->obj->is_dir) ? "folder" : "text-x-generic",
                    KV_COL_KEY, nice_key,
                    KV_COL_LABEL, (label),
                    KV_COL_VALUE, (nice),
                    KV_COL_TAG, (tag),
                    KV_COL_INDEX, (i),
                    KV_COL_LIVE, (is_live),
                    -1);

        g_free(nice);
        g_free(nice_key);

        i++; l = l->next;
    }
}

void bp_sysobj_view_show_inspector(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    priv->show_inspector = TRUE;
    gtk_widget_show(priv->pi);
}

void bp_sysobj_view_hide_inspector(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    priv->show_inspector = FALSE;
    gtk_widget_hide(priv->pi);
}

gboolean bp_sysobj_view_inspector_is_visible(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    return priv->show_inspector;
}

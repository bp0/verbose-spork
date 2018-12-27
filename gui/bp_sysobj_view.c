
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

    guint refresh_timer_timeout_id;
    guint refresh_timer_interval_ms;
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
   KV_COL_VALUE,
   KV_COL_INDEX,
   KV_COL_LIVE,
   KV_N_COLUMNS
};

static const char *kv_col_names[] = {
    "", /* icon */
    "Item",
    "Value",
    "", /* pins index */
    "" /* needs full update */,
};

static void _cleanup(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    g_source_remove(priv->refresh_timer_timeout_id);
    pins_free(priv->pins);
    sysobj_free(priv->obj);
    g_free(priv->new_target);
}

static void _create(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);

    /* tree store and tree view */
    priv->store = gtk_tree_store_new(KV_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
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

static gboolean _check_row(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, bpSysObjViewPrivate *priv) {
    int pi = 0, full = 0, depth = gtk_tree_path_get_depth(path);
    gtk_tree_model_get(model, iter, KV_COL_INDEX, &pi, KV_COL_LIVE, &full, -1);
    const pin *p = pins_get_nth(priv->pins, pi);
    if (!p) return FALSE; //TODO:
    gboolean up = sysobj_read(p->obj, FALSE);
    if (up || full || !sysobj_exists(p->obj) ) {
        if (sysobj_exists(p->obj) ) {
            /* update value */
            gboolean writable = p->obj->others_can_write || p->obj->root_can_write;
            gchar *icon = "folder";
            if (!p->obj->data.is_dir)
                icon = writable ? "application-x-executable" : "text-x-generic";

            gchar *nice = sysobj_format(p->obj, priv->fmt_opts | FMT_OPT_LIST_ITEM);
            gtk_tree_store_set(GTK_TREE_STORE(model), iter,
                KV_COL_ICON, icon,
                KV_COL_KEY, p->obj->name_req,
                KV_COL_VALUE, nice,
                KV_COL_LIVE, 0, -1);
            g_free(nice);
            if (p == bp_pin_inspect_get_pin(BP_PIN_INSPECT(priv->pi)) ) {
                bp_pin_inspect_do(BP_PIN_INSPECT(priv->pi), p, priv->fmt_opts);
            }
            /* check children */
            if (p->obj->data.is_dir && depth <= priv->max_depth) {
                GSList *l = NULL, *childs = sysobj_children(p->obj, NULL, NULL, TRUE);
                for (l = childs; l; l = l->next) {
                    GtkTreeIter iter_new;
                    int npi = pins_add_from_fn(priv->pins, p->obj->path_req, (gchar*)l->data);
                    if (npi >= 0) {
                        /* needs to be added */
                        gtk_tree_store_append(priv->store, &iter_new, iter);
                        gtk_tree_store_set(priv->store, &iter_new,
                                    KV_COL_INDEX, npi,
                                    KV_COL_LIVE, 1,
                                    -1);
                    } /* new row added */
                } /* for each child */
                g_slist_free_full(childs, g_free);
                if (depth == 1)
                    gtk_tree_view_expand_row(GTK_TREE_VIEW(priv->view), path, FALSE);
            } /* if is_dir */
        } else {
            /* stopped existing */
            gtk_tree_store_remove(GTK_TREE_STORE(model), iter);
        } /* exists */
    } /* if up(date) */
    return FALSE; /* continue */
};

static void _check_tree(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s );
    if (!priv->obj) return;
    gtk_tree_model_foreach(GTK_TREE_MODEL(priv->store), (GtkTreeModelForeachFunc)_check_row, priv);
}

static void _reset(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    bp_pin_inspect_do(BP_PIN_INSPECT(priv->pi), NULL, 0);
    /* clear the old */
    gtk_tree_store_clear(priv->store);
    pins_clear(priv->pins);
    if (priv->obj)
        sysobj_free(priv->obj);
    priv->obj = NULL;
}

static gboolean _new_target(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    gboolean is_new = FALSE;

    if (priv->new_target) {
        is_new = TRUE;
        _reset(s);
        /* load the new */
        priv->obj = sysobj_new_from_fn(priv->new_target, NULL);
        g_free(priv->new_target);
        priv->new_target = NULL;
    }

    if (!priv->obj) goto _new_target_finish;
    int npi = pins_add_from_fn(priv->pins, priv->obj->path_req, NULL);

    GtkTreeIter iter;
    if (priv->include_target) {
        GtkTreeIter iter;
        gtk_tree_store_append(priv->store, &iter, NULL);
        gtk_tree_store_set(priv->store, &iter,
                    KV_COL_INDEX, npi,
                    KV_COL_LIVE, 1,
                    -1);
    } else {
        GSList *l = NULL, *childs = sysobj_children(priv->obj, NULL, NULL, TRUE);
        for(l = childs; l; l = l->next) {
            int npi = pins_add_from_fn(priv->pins, priv->obj->path_req, (gchar*)l->data);
            gtk_tree_store_append(priv->store, &iter, NULL);
            gtk_tree_store_set(priv->store, &iter,
                        KV_COL_INDEX, npi,
                        KV_COL_LIVE, 1,
                        -1);
        }
        g_slist_free_full(childs, g_free);
    }

    bp_sysobj_view_refresh(s);

    _select_first_item(s);
    g_signal_emit(s, _signals[SIG_LANDED], 0);

_new_target_finish:
    return G_SOURCE_REMOVE; /* remove from the main loop */
}

gboolean bp_sysobj_view_refresh(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    _check_tree(s);
    guint min_ms = (priv->pins->shortest_interval * 1000.0);
    if (min_ms == 0) min_ms = 9876; /* ~10s for "never" changes */
    if (min_ms < 100) min_ms = 98;  /* ~0.1s minumum */
    if (priv->refresh_timer_interval_ms != min_ms) {
        g_source_remove(priv->refresh_timer_timeout_id);
        priv->refresh_timer_interval_ms = min_ms;
        priv->refresh_timer_timeout_id = g_timeout_add(priv->refresh_timer_interval_ms, (GSourceFunc)bp_sysobj_view_refresh, s);
        //printf("bp_sysobj_view(0x%llx) refresh timer is now %lu ms\n", (long long unsigned)s, (long unsigned)min_ms);
        return G_SOURCE_REMOVE; /* was already removed a few lines ago */
    } else
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

void bp_sysobj_view_set_include_target(bpSysObjView *s, gboolean include_target) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    priv->include_target = include_target;
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
    g_idle_add((GSourceFunc)_new_target, s);
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

const pin *bp_sysobj_view_get_selected_pin(bpSysObjView *s) {
    bpSysObjViewPrivate *priv = BP_SYSOBJ_VIEW_PRIVATE(s);
    return bp_pin_inspect_get_pin(BP_PIN_INSPECT(priv->pi));
}


#include "bp_sysobj_search.h"
#include "bp_sysobj_view.h"
#include "sysobj.h"
#include "sysobj_foreach.h"
#include "pin.h"

enum {
    SIG_ITEM_ACTIVATED,
    SIG_LAST,
};
static guint _signals[SIG_LAST] = { 0 };

static sysobj_filter search_default_filters[] = {
    //{ SO_FILTER_STATIC | SO_FILTER_EXCLUDE,     "/sys/kernel/slab/*", NULL },
    { SO_FILTER_NONE, "", NULL }, /* end of list */
};

/* Forward declarations */
static void _create(bpSysObjSearch *s);
static void _cleanup(bpSysObjSearch *s);

/* Private class member */
#define BP_SYSOBJ_SEARCH_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    BP_SYSOBJ_SEARCH_TYPE, bpSysObjSearchPrivate))

typedef struct _bpSysObjSearchPrivate bpSysObjSearchPrivate;

struct _bpSysObjSearchPrivate {
    GtkWidget *sv;
    GtkWidget *entry;
    GtkWidget *btn_go;
    GtkWidget *btn_stop;
    GtkWidget *spinner;
    gchar *search_result_path;
    gchar *search_class_tag;
    gchar *search_query;
    gboolean now_searching;
    guint searched;
    guint found;
    GThread *search_thread;
    gboolean stop;
    GSList *filters;
};

G_DEFINE_TYPE(bpSysObjSearch, bp_sysobj_search, GTK_TYPE_BOX);

/* Initialize the bpSysObjSearch class */
static void
bp_sysobj_search_class_init(bpSysObjSearchClass *klass)
{
    /* TODO: g_type_class_add_private deprecated in glib 2.58 */

    /* Add private indirection member */
    g_type_class_add_private(klass, sizeof(bpSysObjSearchPrivate));

    /* Signals */
    _signals[SIG_ITEM_ACTIVATED] = g_signal_new ("search-result-activated",
                 G_TYPE_OBJECT,
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                 0 /* class_offset */,
                 NULL /* accumulator */,
                 NULL /* accumulator data */,
                 NULL /* C marshaller */,
                 G_TYPE_NONE /* return_type */,
                 1     /* n_params */,
                 G_TYPE_POINTER);

}

/* Initialize a new bpSysObjSearch instance */
static void
bp_sysobj_search_init(bpSysObjSearch *s)
{
    /* This means that bpSysObjSearch doesn't supply its own GdkWindow */
    gtk_widget_set_has_window(GTK_WIDGET(s), FALSE);
    /* Set redraw on allocate to FALSE if the top left corner of your widget
     * doesn't change when it's resized; this saves time */
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(s), FALSE);

    /* Initialize private members */
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    memset(priv, sizeof(bpSysObjSearchPrivate), 0);

    priv->search_result_path = g_strdup_printf(":app/search/%p", s);
    sysobj_virt_add_simple_mkpath(priv->search_result_path, NULL, "*", VSO_TYPE_DIR);
    priv->search_class_tag = g_strdup_printf("search,%p", s);
    class_add_simple(priv->search_result_path, "Search result set", priv->search_class_tag, OF_NONE, 1.0, NULL);
    priv->now_searching = FALSE;

    GSList *filters = NULL;
    int i = 0;
    while(search_default_filters[i].type != SO_FILTER_NONE) {
        filters = g_slist_append(filters, &search_default_filters[i]);
        i++;
    }
    priv->filters = filters;

    _create(s);

    g_signal_connect(s, "destroy", G_CALLBACK(_cleanup), NULL);
}

/* Return a new bpSysObjSearch cast to a GtkWidget */
GtkWidget *
bp_sysobj_search_new()
{
    return GTK_WIDGET(g_object_new(bp_sysobj_search_get_type(), NULL));
}

static void _results_line_activate(bpSysObjView *view, gchar *sysobj_path, bpSysObjSearch *s) {
    /* resolve the symlink target */
    sysobj *obj = sysobj_new_from_fn(sysobj_path, NULL);
    gchar *target = g_strdup(obj->path);
    sysobj_free(obj);
    g_free(sysobj_path);
    g_signal_emit(s, _signals[SIG_ITEM_ACTIVATED], 0, target);
}

static void _clear_results(bpSysObjSearch *s) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    gchar *glob = g_strdup_printf("%s/*", priv->search_result_path);
    sysobj_virt_remove(glob);
    g_free(glob);
    bp_sysobj_view_refresh(BP_SYSOBJ_VIEW(priv->sv));
}

gboolean _search_examine(sysobj *obj, bpSysObjSearch *s, const sysobj_foreach_stats *stats) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);

    priv->searched++;
    if (!g_strcmp0(priv->search_query, obj->name_req) ) {
        gchar rname[256] = "";
        snprintf(rname, 255, "result%d_(%s)", priv->found, obj->name_req);
        sysobj_virt_add_simple(priv->search_result_path, rname, obj->path_req, VSO_TYPE_SYMLINK);
        priv->found++;
    }

    if (priv->stop || priv->found >= 100)
        return SYSOBJ_FOREACH_STOP;
    else
        return SYSOBJ_FOREACH_CONTINUE;
}

static void _search_func(bpSysObjSearch *s) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    priv->searched = 0;
    priv->found = 0;

    /* begin search */
    sysobj_foreach(priv->filters, (f_sysobj_foreach)_search_examine, s, SO_FOREACH_MT);

    /* done searching */
    priv->now_searching = FALSE;
    gtk_spinner_stop(GTK_SPINNER(priv->spinner));
    gtk_widget_hide(priv->btn_stop);
    gtk_widget_show(priv->btn_go);
}

static void _search_go(GtkButton *button, gpointer user_data) {
    bpSysObjSearch *s = user_data;
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);

    if (priv->now_searching) {
        /* stop a search */
        priv->stop = TRUE;
        g_thread_join(priv->search_thread);
    } else {
        /* start a search */
        gtk_widget_hide(priv->btn_go);
        gtk_widget_show(priv->btn_stop);
        priv->search_query = g_strdup( gtk_entry_get_text(GTK_ENTRY(priv->entry)) );
        gtk_spinner_start(GTK_SPINNER(priv->spinner));
        priv->now_searching = TRUE;
        priv->stop = FALSE;
        _clear_results(s);
        priv->search_thread = g_thread_new("searcher", (GThreadFunc)_search_func, s);
    }
}

static void _query_entry_activate(GtkEntry *entry, gpointer user_data) {
    bpSysObjSearch *s = user_data;
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    _search_go(GTK_BUTTON(priv->btn_go), s);
}

static void _cleanup(bpSysObjSearch *s) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    if (priv->now_searching) {
        priv->stop = TRUE;
        g_thread_join(priv->search_thread);
    }
    sysobj_filter_free_list(priv->filters);
    g_free(priv->search_result_path);
    g_free(priv->search_class_tag);
    g_free(priv->search_query);
}

void _create(bpSysObjSearch *s) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);

    GtkWidget *btn_go = gtk_button_new_from_icon_name("go-next", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_go, _("Search"));
    GtkWidget *btn_stop = gtk_button_new_from_icon_name("process-stop", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_stop, _("Stop search"));

    GtkWidget *btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    priv->entry = gtk_entry_new();
    GtkWidget *spinner = gtk_spinner_new();
    gtk_box_pack_start (GTK_BOX (btns), priv->entry, TRUE, TRUE, 0); gtk_widget_show (priv->entry);
    gtk_box_pack_start (GTK_BOX (btns), btn_go, FALSE, FALSE, 0); gtk_widget_show (btn_go);
    gtk_box_pack_start (GTK_BOX (btns), btn_stop, FALSE, FALSE, 0); gtk_widget_hide (btn_stop);
    gtk_box_pack_start (GTK_BOX (btns), spinner, FALSE, FALSE, 10); gtk_widget_show (spinner);

    priv->sv = bp_sysobj_view_new();
    bp_sysobj_view_set_fmt_opts(BP_SYSOBJ_VIEW(priv->sv), FMT_OPT_PANGO | FMT_OPT_NO_JUNK);
    bp_sysobj_view_hide_inspector(BP_SYSOBJ_VIEW(priv->sv));
    bp_sysobj_view_set_path(BP_SYSOBJ_VIEW(priv->sv), priv->search_result_path);
    //bp_sysobj_view_set_include_target(FALSE);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(s), GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start (GTK_BOX(s), btns, FALSE, FALSE, 5); gtk_widget_show(btns);
    gtk_box_pack_start (GTK_BOX(s), priv->sv, TRUE, TRUE, 5); gtk_widget_show(priv->sv);

    priv->btn_go = btn_go;
    priv->btn_stop = btn_stop;
    priv->spinner = spinner;

    /* buttons */
    g_signal_connect (priv->entry, "activate",
          G_CALLBACK (_query_entry_activate), s);
    g_signal_connect (btn_go, "clicked",
          G_CALLBACK (_search_go), s);
    g_signal_connect (btn_stop, "clicked",
          G_CALLBACK (_search_go), s);
    /* sysobj view */
    g_signal_connect (priv->sv, "item-activated",
          G_CALLBACK (_results_line_activate), s);
}


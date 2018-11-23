
#include "bp_sysobj_search.h"
#include "bp_sysobj_view.h"
#include "sysobj.h"
#include "pin.h"

enum {
    SIG_ITEM_ACTIVATED,
    SIG_LAST,
};
static guint _signals[SIG_LAST] = { 0 };

static const struct {gchar *label, *path;} places_list[] = {
    /* label = path if label is null */
    { NULL, "/sys" },
    { "vsysfs", ":" },

    { NULL, "/sys/devices/system/cpu" },
    { NULL, "/proc/sys" },
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
    gchar *search_query;
    gboolean now_searching;
    GThread *search_thread;
    gboolean stop;
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

    priv->search_result_path = g_strdup_printf(":app/search/0x%016llx", (long long unsigned)s);
    sysobj_virt_add_simple_mkpath(priv->search_result_path, NULL, "*", VSO_TYPE_DIR);
    class_add_simple(priv->search_result_path, "Search result set", priv->search_result_path, OF_NONE, 1.0);
    priv->now_searching = FALSE;

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

static GList* __push_if_uniq(GList *l, gchar *base, gchar *name) {
    gchar *path = name
        ? g_strdup_printf("%s/%s", base, name)
        : g_strdup(base);
    GList *a = NULL;
    for(a = l; a; a = a->next) {
        if (!g_strcmp0((gchar*)a->data, path) ) {
            g_free(path);
            return l;
        }
    }
    return g_list_append(l, path);
}

static GList* __shift(GList *l, gchar **s) {
    *s = (gchar*)l->data;
    return g_list_delete_link(l, l);
}

static void _clear_results(bpSysObjSearch *s) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    gchar *glob = g_strdup_printf("%s/*", priv->search_result_path);
    sysobj_virt_remove(glob);
    g_free(glob);
}

static void _search_func(bpSysObjSearch *s) {
    bpSysObjSearchPrivate *priv = BP_SYSOBJ_SEARCH_PRIVATE(s);
    int found = 0;

    _clear_results(s);

    GList *searched_paths = NULL;
    GList *to_search = NULL;
    to_search = __push_if_uniq(to_search, ":/", NULL);

    gchar *path = NULL;
    while(!priv->stop && g_list_length(to_search) && found < 100) {
        to_search = __shift(to_search, &path);
        //printf("found:%d stop:%d to_search:%d searched:%d now: %s\n", found, priv->stop ? 1 : 0, g_list_length(to_search), g_list_length(searched_paths), path );
        sysobj *obj = sysobj_new_from_fn(path, NULL);
        if (!g_list_find_custom(searched_paths, obj->path, (GCompareFunc)g_strcmp0) ) {
            searched_paths = __push_if_uniq(searched_paths, obj->path, NULL);
            if (obj) {
                /* check for match */
                if (strstr(obj->name_req, priv->search_query) ) {
                    sysobj_virt_add_simple(priv->search_result_path, obj->name_req, obj->path_req, VSO_TYPE_SYMLINK);
                    found++;
                }
                /* scan children */
                if (obj->data.is_dir) {
                    sysobj_read(obj, FALSE);
                    GSList *lc = obj->data.childs;
                    for (lc = obj->data.childs; lc; lc = lc->next) {
                        to_search = __push_if_uniq(to_search, obj->path_req, (gchar*)lc->data);
                    }
                }
            }
        }
        sysobj_free(obj);
    }
    /* done searching */
    g_list_free_full(searched_paths, g_free);
    g_list_free_full(to_search, g_free);

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
    g_free(priv->search_result_path);
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


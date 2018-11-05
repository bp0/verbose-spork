
#include "bsysinfo-gtk.h"

GtkWidget *notebook = NULL;
#define PAGE_BROWSER 0

#define UPDATE_TIMER_SECONDS 0.2

const char about_text[] =
    "Another system info thing\n"
    "(c) 2018 Burt P. <pburt0@gmail.com>\n"
    "https://github.com/bp0/verbose-spork\n"
    "\n";

static int app_init(void) {
    sysobj_init(NULL);
    return 1;
}

static void app_cleanup(void) {
    sysobj_cleanup();
}

struct {
    gint timeout_id;
    gint interval_ms;
} refresh_timer;

enum
{
   KV_COL_KEY,
   KV_COL_LABEL,
   KV_COL_VALUE,
   KV_COL_TAG,
   KV_COL_INDEX,
   KV_COL_LIVE,
   KV_N_COLUMNS
};

char *kv_col_names[] = {
    "Item",
    "Label",
    "Value",
    "Tag",
    "Index",
    "Is Live",
};

typedef struct {
    const pin *p;
    GtkWidget *container;
    GtkWidget *box;
} pin_inspect;

pin_inspect *pin_inspect_create() {
    pin_inspect *pi = g_new0(pin_inspect, 1);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    //gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_widget_show(box);

    pi->box = box;
    pi->container = scroll;

    return pi;
}

void pin_inspect_do(pin_inspect *pi, const pin *p, int fmt_opts) {
    pi->p = p;

    /* clear */
    GList *children, *iter;
    children = gtk_container_get_children(GTK_CONTAINER(pi->box));
    for(iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    if (!p) return;

    /* new */
    gchar *label = g_strdup(sysobj_label(p->obj));
    gchar *nice = sysobj_format(p->obj, fmt_opts);
    gchar *tag = g_strdup_printf("{%s}", p->obj->cls ? (p->obj->cls->tag ? p->obj->cls->tag : p->obj->cls->pattern) : "none");
    gchar *update = g_strdup_printf("update_interval = %0.4lfs, last_update = %0.4lfs", p->update_interval, p->last_update);

    if (!label)
        label = g_strdup("");

    gchar *mt = g_strdup_printf(
    /* name   */ "<big><big>%s</big></big>\n"
    /* label  */ "%s\n"
    /* tag    */ "%s\n"
    /* value  */ "%s\n"
    /* resolv */ "%s\n"
    /* update */ "%s\n",
        p->obj->name,
        label, tag, nice, p->obj->path, update);

    g_free(label);
    g_free(nice);
    g_free(tag);
    g_free(update);

    GtkWidget *lbl;
    lbl = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(lbl), TRUE);
    gtk_label_set_markup(GTK_LABEL(lbl), mt);
    gtk_widget_show(lbl);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl, 10);
    gtk_box_pack_start(GTK_BOX(pi->box), lbl, TRUE, TRUE, 0);
}

typedef struct {
    pin_list *pins;
    gboolean as_tree;
    GtkTreeStore *store;
    GtkWidget *view;
    GtkWidget *container;
    int fmt_opts;
    pin_inspect *pinspect;
    GtkWidget *btn_watch; /* */
} pins_list_view;

void browser_navigate(const gchar *new_location);
void watchlist_add(gchar *path);

void pins_list_view_select(GtkTreeView *tree_view, gpointer user_data) {
    GtkTreeIter iter;
    GtkTreePath *path;
    pins_list_view *plv = (pins_list_view*)user_data;
    int pi = -1;

    gtk_tree_view_get_cursor(tree_view, &path, NULL);
    if (path) {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(plv->store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(plv->store), &iter, KV_COL_INDEX, &pi, -1);
        if (pi >= 0) {
            pin *p = pins_get_nth(plv->pins, pi);
            pin_inspect_do(plv->pinspect, p, plv->fmt_opts);

            /* disable watchlist add button if new location is already watched
            if (plv->btn_watch) {
                pin *wp = pins_find_by_path(pwl, p->obj->path);
                if (wp)
                    gtk_widget_set_sensitive (plv->btn_watch, FALSE);
                else
                    gtk_widget_set_sensitive (plv->btn_watch, TRUE);
            }*/
        }
    } else
        pin_inspect_do(plv->pinspect, NULL, 0);
}

void pins_list_view_activate
            (GtkTreeView       *tree_view,
               GtkTreePath       *path,
               GtkTreeViewColumn *column,
               gpointer           user_data) {

    GtkTreeIter iter;
    pins_list_view *plv = (pins_list_view*)user_data;
    int pi = -1;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(plv->store), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(plv->store), &iter, KV_COL_INDEX, &pi, -1);
    if (pi >= 0) {
        pin *p = pins_get_nth(plv->pins, pi);
        browser_navigate(p->obj->path_req);
    }
}

static pins_list_view *pins_list_view_create(gboolean as_tree) {
    pins_list_view *plv = g_new0(pins_list_view, 1);
    plv->pins = pins_new();
    plv->as_tree = as_tree;

    plv->store = gtk_tree_store_new(KV_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    plv->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(plv->store));

    GtkWidget *list_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(list_scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(list_scroll), plv->view);
    gtk_widget_show(plv->view);

    plv->pinspect = pin_inspect_create();

    plv->container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (plv->container), list_scroll, TRUE, TRUE, 0); gtk_widget_show (list_scroll);
    gtk_box_pack_start (GTK_BOX (plv->container), plv->pinspect->container, TRUE, TRUE, 5); gtk_widget_show (plv->pinspect->container);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    /* only name and value */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes
        (kv_col_names[KV_COL_KEY], renderer, "text", KV_COL_KEY, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (plv->view), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes
        (kv_col_names[KV_COL_VALUE], renderer, "markup", KV_COL_VALUE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (plv->view), column);

    plv->fmt_opts = FMT_OPT_NONE;

    g_signal_connect (plv->view, "cursor-changed",
          G_CALLBACK (pins_list_view_select), plv);

    g_signal_connect (plv->view, "row-activated",
          G_CALLBACK (pins_list_view_activate), plv);

    return plv;
}

static void pins_list_view_fill(pins_list_view *plv) {
    GtkTreeIter iter, parent;

    gtk_tree_store_clear(plv->store);

    int i = 0;
    GSList *l = plv->pins->list;
    while (l) {
        pin *p = l->data;

        gchar *label = g_strdup(sysobj_label(p->obj));
        gchar *nice = sysobj_format(p->obj, plv->fmt_opts);
        gchar *tag = g_strdup_printf("{%s}", p->obj->cls ? (p->obj->cls->tag ? p->obj->cls->tag : p->obj->cls->pattern) : "none");
        gboolean is_live = !(p->update_interval == UPDATE_INTERVAL_NEVER);

        if (plv->as_tree) {
            if (i == 0)
                gtk_tree_store_append(plv->store, &parent, NULL);
            else
                gtk_tree_store_append(plv->store, &iter, &parent);

            gtk_tree_store_set(plv->store, i ? &iter : &parent,
                        KV_COL_KEY, (p->obj->name_req),
                        KV_COL_LABEL, (label),
                        KV_COL_VALUE, (nice),
                        KV_COL_TAG, (tag),
                        KV_COL_INDEX, (i),
                        KV_COL_LIVE, (is_live),
                        -1);
        } else {
            gtk_tree_store_append(plv->store, &iter, NULL);
            gtk_tree_store_set(plv->store, &iter,
                        KV_COL_KEY, (p->obj->name_req),
                        KV_COL_LABEL, (label),
                        KV_COL_VALUE, (nice),
                        KV_COL_TAG, (tag),
                        KV_COL_INDEX, (i),
                        KV_COL_LIVE, (is_live),
                        -1);
        }
        g_free(nice);

        i++; l = l->next;
    }
}

//TODO: handle as_tree
void pins_list_view_append(pins_list_view *plv, const gchar *path) {
    GtkTreeIter iter;
    int i = pins_add_from_fn(plv->pins, path, NULL);
    if (i>=0) {
        pin *p = pins_get_nth(plv->pins, i);
        const gchar *label = sysobj_label(p->obj);
        gchar *nice = sysobj_format(p->obj, plv->fmt_opts);
        gtk_tree_store_append(plv->store, &iter, NULL);
        gtk_tree_store_set(plv->store, &iter,
                    KV_COL_KEY, (p->obj->name_req),
                    KV_COL_VALUE, (nice),
                    KV_COL_INDEX, (i),
                    KV_COL_LIVE, (!(p->update_interval == UPDATE_INTERVAL_NEVER)),
                    -1);
    }
}

gboolean pins_list_view_update_row(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    int pi, live;
    const pin *p = NULL;
    pins_list_view *plv = data;
    gtk_tree_model_get(model, iter, KV_COL_LIVE, &live, -1);
    if (live) {
        gtk_tree_model_get(model, iter, KV_COL_INDEX, &pi, -1);
        p = pins_pin_if_updated_since(plv->pins, pi, UPDATE_TIMER_SECONDS);
        if (p) {
            gchar *nice = sysobj_format(p->obj, plv->fmt_opts);
            gtk_tree_store_set(GTK_TREE_STORE(model), iter, KV_COL_VALUE, nice, -1);
            g_free(nice);
            if (plv->pinspect->p == p)
                pin_inspect_do(plv->pinspect, p, plv->fmt_opts);
        }
    }
    return FALSE;
}

static void pins_list_view_update(pins_list_view *plv) {
    pins_refresh(plv->pins);
    gtk_tree_model_foreach(GTK_TREE_MODEL(plv->store), pins_list_view_update_row, plv);
}

static void add_notebook_page(const gchar *label, GtkWidget *notebook, GtkWidget *page_widget, gint border) {
    GtkWidget *lbl = gtk_label_new (label);
    gtk_container_set_border_width (GTK_CONTAINER (page_widget), border);
    gtk_widget_set_size_request (page_widget, 100, 75);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), page_widget, lbl);
    gtk_widget_show (page_widget);
}

struct {
    GtkWidget *container;
    pins_list_view *plv;
} gwl;

void watchlist_init() {
    gwl.plv = pins_list_view_create(FALSE);
    gwl.container = gwl.plv->container;
}

void watchlist_add(gchar *path) {
    pins_list_view_append(gwl.plv, path);
}

void watchlist_cleanup() {
    pins_free(gwl.plv->pins);
}

struct {
    GtkWidget *container;
    GtkWidget *query;
    GtkWidget *btn_watch;
    pins_list_view *browser;
    GSList *history;
    int h_pos; /* index into history */
    int h_len; /* g_slist_lenght(.history) */
} gel;

void browser_activate(GtkEntry *entry, gpointer  user_data) {
    const gchar *loc = gtk_entry_get_text(entry);
    browser_navigate(loc);
}

void browser_watch (GtkButton *button, gpointer user_data) {
    watchlist_add(gel.browser->pinspect->p->obj->path);
}

void browser_refresh (GtkButton *button, gpointer user_data) {
    GSList *li = g_slist_nth(gel.history, gel.h_pos);
    browser_navigate(li->data);
}

void browser_up (GtkButton *button, gpointer user_data) {
    if (gel.h_pos >= 0) {
        GSList *li = g_slist_nth(gel.history, gel.h_pos);
        if (strchr(li->data+1, '/')) {
            gchar *pp = g_path_get_dirname(li->data);
            browser_navigate(pp);
        }
    }
}

void browser_back (GtkButton *button, gpointer user_data) {
    DEBUG("history len = %u, current = %d", gel.h_len, gel.h_pos);
    gel.h_pos++;
    if (gel.h_pos >= gel.h_len)
        gel.h_pos = gel.h_len-1;
    else {
        GSList *li = g_slist_nth(gel.history, gel.h_pos);
        browser_navigate(li->data);
    }
}

void browser_init() {
    GtkWidget *btn_back = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_back, _("Back"));
    GtkWidget *btn_up = gtk_button_new_from_icon_name("go-up", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_up, _("Parent"));
    GtkWidget *btn_ref = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_ref, _("Reload"));
    GtkWidget *btn_watch = gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_watch, _("Add to Watchlist"));
    GtkWidget *btns = gel.container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gel.query = gtk_entry_new();
    gtk_box_pack_start (GTK_BOX (btns), btn_back, FALSE, FALSE, 0); gtk_widget_show (btn_back);
    gtk_box_pack_start (GTK_BOX (btns), btn_up, FALSE, FALSE, 0); gtk_widget_show (btn_up);
    gtk_box_pack_start (GTK_BOX (btns), btn_ref, FALSE, FALSE, 5); gtk_widget_show (btn_ref);
    gtk_box_pack_start (GTK_BOX (btns), gel.query, TRUE, TRUE, 0); gtk_widget_show (gel.query);
    gtk_box_pack_start (GTK_BOX (btns), btn_watch, FALSE, FALSE, 5); gtk_widget_show (btn_watch);
    gel.browser = pins_list_view_create(TRUE);
    gel.browser->fmt_opts = FMT_OPT_PANGO | FMT_OPT_NO_JUNK;
    gel.container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (gel.container), btns, FALSE, FALSE, 5); gtk_widget_show (btns);
    gtk_box_pack_start (GTK_BOX (gel.container), gel.browser->container, TRUE, TRUE, 5); gtk_widget_show (gel.browser->container);

    gel.btn_watch = btn_watch;

    g_signal_connect (gel.query, "activate",
          G_CALLBACK (browser_activate), NULL);
    g_signal_connect (btn_back, "clicked",
          G_CALLBACK (browser_back), NULL);
    g_signal_connect (btn_up, "clicked",
          G_CALLBACK (browser_up), NULL);
    g_signal_connect (btn_ref, "clicked",
          G_CALLBACK (browser_refresh), NULL);
    g_signal_connect (btn_watch, "clicked",
          G_CALLBACK (browser_watch), NULL);

    gel.h_len = 0;
    gel.h_pos = -1;
}

void browser_cleanup() {
    pins_free(gel.browser->pins);
    g_slist_free_full(gel.history, g_free);
}

void browser_navigate(const gchar *new_location) {
    const gchar *fn;
    gchar *safe = g_strdup(new_location);
    gchar *landed = NULL; /* becomes owned by history */

    DEBUG("browser_navigate( %s )...", safe);

    gtk_entry_set_text(GTK_ENTRY(gel.query), safe);

    pins_clear(gel.browser->pins);
    gtk_tree_store_clear(gel.browser->store);

    sysobj *ex_obj = sysobj_new_from_fn(safe, NULL);
    g_free(safe);

    pins_add_from_fn(gel.browser->pins, ex_obj->path_req, NULL);
    if (ex_obj->exists) {
        if (ex_obj->is_dir) {
            GSList *l = NULL, *childs = sysobj_children_ex(ex_obj, NULL, NULL, TRUE);
            l = childs;
            while(l) {
                fn = l->data;
                DEBUG("... %s %s", ex_obj->path, fn);
                pins_add_from_fn(gel.browser->pins, ex_obj->path_req, fn);
                l = l->next;
            }
            g_slist_free_full(childs, g_free);
        }
    }
    landed = g_strdup(ex_obj->path_req); /* perhaps redirected */
    sysobj_free(ex_obj);

    DEBUG("history len = %d, pos: %d", gel.h_len, gel.h_pos);

    gtk_entry_set_text(GTK_ENTRY(gel.query), landed);
    if (gel.h_len > 0) {
        GSList *li = g_slist_nth(gel.history, gel.h_pos);
        if (strcmp(landed, li->data) != 0)
            gel.history = g_slist_insert(gel.history, landed, gel.h_pos);
        else
            g_free(landed);
        gel.h_len = g_slist_length(gel.history);
    } else {
        gel.history = g_slist_prepend(gel.history, landed);
        gel.h_len = 1;
        gel.h_pos = 0;
    }

    pins_list_view_fill(gel.browser);
    gtk_tree_view_expand_all(GTK_TREE_VIEW(gel.browser->view));

    /* select first item */
    GtkTreePath *path = gtk_tree_path_new_from_string("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW(gel.browser->view),
                path, NULL, FALSE);

    if (notebook)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), PAGE_BROWSER);
}

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    widget = widget; /* to avoid a warning */
    event = event; /* to avoid a warning */
    data = data; /* to avoid a warning */
    return FALSE;
}

static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    widget = widget; /* to avoid a warning */
    data = data; /* to avoid a warning */
    g_source_remove(refresh_timer.timeout_id);
    watchlist_cleanup();
    browser_cleanup();
    app_cleanup();
    gtk_main_quit();
}

GtkWidget *about;

void about_init() {
    gchar *glib_version_info =
        g_strdup_printf("GLib %d.%d.%d (built against: %d.%d.%d)",
            glib_major_version, glib_minor_version, glib_micro_version,
            GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION );
    gchar *gtk_version_info =
        g_strdup_printf("GTK %d.%d.%d (built against: %d.%d.%d)",
            gtk_major_version, gtk_minor_version, gtk_micro_version,
            GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION );

    gchar *text = g_strdup_printf("\n%s%s\n%s\n", about_text, glib_version_info, gtk_version_info);

    GtkWidget *lbl;
    lbl = gtk_label_new(NULL);
    gtk_label_set_text(GTK_LABEL(lbl), text);
    gtk_label_set_line_wrap(GTK_LABEL(lbl), TRUE);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl, 10);
    gtk_widget_show(lbl);

    about = lbl;

    g_free(glib_version_info);
    g_free(gtk_version_info);
}

static gboolean refresh_data(gpointer data) {
    data = data; /* to avoid a warning */

    pins_list_view_update(gel.browser);
    pins_list_view_update(gwl.plv);

    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    const gchar *altroot = NULL;
    const gchar *query = NULL;

    if (argc == 2)
        query = argv[1];
    if (argc > 2) {
        altroot = argv[1];
        query = argv[2];
    }
    if (altroot)
        sysobj_root_set(altroot);

    gtk_init (&argc, &argv);
    app_init();
    about_init();
    browser_init();
    watchlist_init();

    if (DEBUG_BUILD)
        class_dump_list();

    refresh_timer.interval_ms = (UPDATE_TIMER_SECONDS * 1000.0);
    refresh_timer.timeout_id = g_timeout_add(refresh_timer.interval_ms, refresh_data, NULL);

    /* Set up main window */
    GtkWidget *window;
    GtkWidget *mbox;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect (window, "delete-event",
          G_CALLBACK (delete_event), NULL);
    g_signal_connect (window, "destroy",
          G_CALLBACK (destroy), NULL);

    /* Sets the border width of the window. */
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    /* notebook pages */
    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    add_notebook_page(_("Browser"), notebook, gel.container, 5);
    add_notebook_page(_("Watchlist"), notebook, gwl.container, 5);
    add_notebook_page(_("About"), notebook, about, 5);

    if (query) {
        browser_navigate(query);
    } else {
        //browser_navigate("/sys/class/dmi/id");
        browser_navigate("/sys/devices/system/cpu");
    }

    watchlist_add("/sys/devices/system/cpu/cpu0");
    watchlist_add("/sys/devices/system/cpu/cpu1");
    watchlist_add("/sys/devices/system/cpu/cpu2");
    watchlist_add("/sys/devices/system/cpu/cpu3");

    /* This packs the notebook into the window (a gtk container). */
    gtk_container_add (GTK_CONTAINER (window), notebook);
    gtk_widget_show (notebook);

    gtk_window_resize (GTK_WINDOW (window), 740, 400);
    gtk_widget_show (window);

    gtk_main ();

    return 0;
}

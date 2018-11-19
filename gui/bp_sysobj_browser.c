
#include "bp_sysobj_browser.h"
#include "bp_sysobj_view.h"
#include "sysobj.h"
#include "pin.h"

static const struct {gchar *label, *path;} places_list[] = {
    /* label = path if label is null */
    { NULL, "/sys" },
    { "vsysfs", ":" },

    { NULL, "/sys/devices/system/cpu" },
    { NULL, "/proc/sys" },
};

/* Forward declarations */
static void _create(bpSysObjBrowser *s);
static void _cleanup(bpSysObjBrowser *s);

/* Private class member */
#define BP_SYSOBJ_BROWSER_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    BP_SYSOBJ_BROWSER_TYPE, bpSysObjBrowserPrivate))

typedef struct _bpSysObjBrowserPrivate bpSysObjBrowserPrivate;

struct _bpSysObjBrowserPrivate {
    GtkWidget *sv;
    GtkWidget *query;
    GtkWidget *btn_watch;
    GtkWidget *browser;
    GSList *history;
    int h_pos; /* index into history */
    int h_len; /* g_slist_lenght(.history) */
};

G_DEFINE_TYPE(bpSysObjBrowser, bp_sysobj_browser, GTK_TYPE_BOX);

/* Initialize the bpSysObjBrowser class */
static void
bp_sysobj_browser_class_init(bpSysObjBrowserClass *klass)
{
    /* TODO: g_type_class_add_private deprecated in glib 2.58 */

    /* Add private indirection member */
    g_type_class_add_private(klass, sizeof(bpSysObjBrowserPrivate));
}

/* Initialize a new bpSysObjBrowser instance */
static void
bp_sysobj_browser_init(bpSysObjBrowser *s)
{
    /* This means that bpSysObjBrowser doesn't supply its own GdkWindow */
    gtk_widget_set_has_window(GTK_WIDGET(s), FALSE);
    /* Set redraw on allocate to FALSE if the top left corner of your widget
     * doesn't change when it's resized; this saves time */
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(s), FALSE);

    /* Initialize private members */
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);
    memset(priv, sizeof(bpSysObjBrowserPrivate), 0);

    _create(s);

    g_signal_connect(s, "destroy", G_CALLBACK(_cleanup), NULL);
}

/* Return a new bpSysObjBrowser cast to a GtkWidget */
GtkWidget *
bp_sysobj_browser_new()
{
    return GTK_WIDGET(g_object_new(bp_sysobj_browser_get_type(), NULL));
}

void bp_sysobj_browser_navigate(bpSysObjBrowser *s, const gchar *new_location) {
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);

    gchar *safe = g_strdup(new_location);
    gtk_entry_set_text(GTK_ENTRY(priv->query), safe);
    if (priv->h_len > 0) {
        GSList *li = g_slist_nth(priv->history, priv->h_pos);
        if (strcmp(safe, li->data) != 0)
            priv->history = g_slist_insert(priv->history, safe, priv->h_pos);
        else
            g_free(safe);
        priv->h_len = g_slist_length(priv->history);
    } else {
        priv->history = g_slist_prepend(priv->history, safe);
        priv->h_len = 1;
        priv->h_pos = 0;
    }

    bp_sysobj_view_set_path(BP_SYSOBJ_VIEW(priv->sv), new_location);
}

void _browser_line_activate(bpSysObjView *view, const gchar *sysobj_path, bpSysObjBrowser *s) {
    bp_sysobj_browser_navigate(s, sysobj_path);
}

static void _browser_entry_activate(GtkEntry *entry, gpointer user_data) {
    bpSysObjBrowser *s = user_data;
    const gchar *loc = gtk_entry_get_text(entry);
    bp_sysobj_browser_navigate(s, loc);
}

static void _browser_watch(GtkButton *button, gpointer user_data) {
    bpSysObjBrowser *s = user_data;
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);
    const pin *p = bp_sysobj_view_get_selected_pin(BP_SYSOBJ_VIEW(priv->sv));
    if (!p) return;
    gchar *name = g_path_get_basename(p->obj->path);
    sysobj_virt_add_simple(":app/watchlist/no-group", name, p->obj->path,
        VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
}

static void _browser_refresh (GtkButton *button, gpointer user_data) {
    bpSysObjBrowser *s = user_data;
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);

    GSList *li = g_slist_nth(priv->history, priv->h_pos);
    bp_sysobj_browser_navigate(s, li->data);
}

static void _browser_up (GtkButton *button, gpointer user_data) {
    bpSysObjBrowser *s = user_data;
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);
    if (priv->h_pos >= 0) {
        GSList *li = g_slist_nth(priv->history, priv->h_pos);
        if (strchr(li->data+1, '/')) {
            gchar *pp = g_path_get_dirname(li->data);
            bp_sysobj_browser_navigate(s, pp);
        }
    }
}

static void _browser_back (GtkButton *button, gpointer user_data) {
    bpSysObjBrowser *s = user_data;
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);
    //DEBUG("history len = %u, current = %d", gel.h_len, gel.h_pos);
    priv->h_pos++;
    if (priv->h_pos >= priv->h_len)
        priv->h_pos = priv->h_len-1;
    else {
        GSList *li = g_slist_nth(priv->history, priv->h_pos);
        bp_sysobj_browser_navigate(s, li->data);
    }
}

static void _places_select(GtkMenuItem *menuitem, bpSysObjBrowser *s) {
    const gchar *name = gtk_menu_item_get_label(menuitem);
    for (int i = 0; i < (int)G_N_ELEMENTS(places_list); i++) {
        if (places_list[i].label && !strcmp(places_list[i].label, name) ) {
            bp_sysobj_browser_navigate(s, places_list[i].path);
            return;
        } else if (!strcmp(places_list[i].path, name)) {
            bp_sysobj_browser_navigate(s, places_list[i].path);
            return;
        }
    }
}

static void _cleanup(bpSysObjBrowser *s) {
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);
    g_slist_free_full(priv->history, g_free);
}

void _create(bpSysObjBrowser *s) {
    bpSysObjBrowserPrivate *priv = BP_SYSOBJ_BROWSER_PRIVATE(s);

    int i = 0;
    GtkWidget *btn_back = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_back, _("Back"));
    GtkWidget *btn_up = gtk_button_new_from_icon_name("go-up", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_up, _("Parent"));
    GtkWidget *btn_ref = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_ref, _("Reload"));
    GtkWidget *btn_watch = gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_watch, _("Add to Watchlist"));

    GtkWidget *places_menu = gtk_menu_new();
    GtkWidget *mitem;
    for (i = 0; i < (int)G_N_ELEMENTS(places_list); i++) {
        mitem = gtk_menu_item_new_with_label(places_list[i].label ? places_list[i].label : places_list[i].path);
        gtk_menu_shell_append (GTK_MENU_SHELL(places_menu), mitem);
        g_signal_connect(mitem, "activate", G_CALLBACK(_places_select), s);
        gtk_widget_show(mitem);
    }

    GtkWidget *btn_places = gtk_menu_button_new();
    gtk_widget_set_tooltip_text(btn_places, _("Places"));
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(btn_places), places_menu);

    GtkWidget *btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    priv->query = gtk_entry_new();
    gtk_box_pack_start (GTK_BOX (btns), btn_back, FALSE, FALSE, 0); gtk_widget_show (btn_back);
    gtk_box_pack_start (GTK_BOX (btns), btn_up, FALSE, FALSE, 0); gtk_widget_show (btn_up);
    gtk_box_pack_start (GTK_BOX (btns), btn_ref, FALSE, FALSE, 5); gtk_widget_show (btn_ref);
    gtk_box_pack_start (GTK_BOX (btns), priv->query, TRUE, TRUE, 0); gtk_widget_show (priv->query);
    gtk_box_pack_start (GTK_BOX (btns), btn_places, FALSE, FALSE, 5); gtk_widget_show (btn_places);
    gtk_box_pack_start (GTK_BOX (btns), btn_watch, FALSE, FALSE, 5); gtk_widget_show (btn_watch);

    priv->sv = bp_sysobj_view_new();
    bp_sysobj_view_set_fmt_opts(BP_SYSOBJ_VIEW(priv->sv), FMT_OPT_PANGO | FMT_OPT_NO_JUNK);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(s), GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start (GTK_BOX(s), btns, FALSE, FALSE, 5); gtk_widget_show(btns);
    gtk_box_pack_start (GTK_BOX(s), priv->sv, TRUE, TRUE, 5); gtk_widget_show(priv->sv);

    priv->btn_watch = btn_watch;

    /* buttons */
    g_signal_connect (priv->query, "activate",
          G_CALLBACK (_browser_entry_activate), s);
    g_signal_connect (btn_back, "clicked",
          G_CALLBACK (_browser_back), s);
    g_signal_connect (btn_up, "clicked",
          G_CALLBACK (_browser_up), s);
    g_signal_connect (btn_ref, "clicked",
          G_CALLBACK (_browser_refresh), s);
    g_signal_connect (btn_watch, "clicked",
          G_CALLBACK (_browser_watch), s);
    /* sysobj view */
    g_signal_connect (priv->sv, "item-activated",
          G_CALLBACK (_browser_line_activate), s);

    /* history init */
    priv->h_len = 0;
    priv->h_pos = -1;
}


/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "bsysinfo-gtk.h"
#include <inttypes.h> /* for PRIu64 */
#include "uri_handler.h"
#include "bp_sysobj_search.h"
#include "bp_sysobj_browser.h"
#include "bp_sysobj_view.h"

/* use notebook or stack/switcher (gtk3.10+) */
#define USE_NOTEBOOK TRUE
#define PAGE_BROWSER 0 /* when using notebook */

GtkWidget *app_window = NULL;
GtkWidget *notebook = NULL;
GtkWidget *stack = NULL;

GtkWidget *browser = NULL;
GtkWidget *watchlist = NULL;
GtkWidget *search = NULL;
GtkWidget *about = NULL;

const char about_markup_text[] =
    "Another system info thing\n"
    "(c) 2018 Burt P. &lt;pburt0@gmail.com&gt;\n"
    "<a href=\"https://github.com/bp0/verbose-spork\">https://github.com/bp0/verbose-spork</a>\n"
    "\n"
    "<a href=\"sysobj::sysobj\">sysobj</a>\n";

/*
static const gchar default_watchlist[] =
    ":/meminfo/MemTotal=\n"
    ":/meminfo/MemFree=\n"
    "/sys/devices/system/cpu/cpu0=\n"
    "/sys/devices/system/cpu/cpu1=\n"
    "/sys/devices/system/cpu/cpu2=\n"
    "/sys/devices/system/cpu/cpu3=\n";
*/

void browser_navigate(const gchar *new_location);
void watchlist_add(const gchar *path);

gboolean uri_sysobj(gchar *uri) {
    DEBUG("activate link: %s", uri);
    if (g_str_has_prefix(uri, "sysobj:")) {
        browser_navigate(g_utf8_strchr(uri, 7, ':') + 1);
        return TRUE;
    }
    return FALSE; /* didn't handle it */
}

static gboolean about_activate_link (GtkLabel *label, gchar *uri, gpointer user_data) {
    return uri_open(uri);
}

void about_init() {
    gchar *glib_version_info =
        g_strdup_printf("GLib %d.%d.%d (built against: %d.%d.%d)",
            glib_major_version, glib_minor_version, glib_micro_version,
            GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION );
    gchar *gtk_version_info =
        g_strdup_printf("GTK %d.%d.%d (built against: %d.%d.%d)",
            gtk_major_version, gtk_minor_version, gtk_micro_version,
            GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION );

    gchar *text = g_strdup_printf("\n%s%s\n%s\n", about_markup_text, glib_version_info, gtk_version_info);

    GtkWidget *lbl;
    lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl), text);
    gtk_label_set_line_wrap(GTK_LABEL(lbl), TRUE);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl, 10);
    g_signal_connect(lbl, "activate-link", G_CALLBACK(about_activate_link), NULL);
    gtk_widget_show(lbl);

    about = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX(about), lbl, TRUE, TRUE, 0); gtk_widget_show(lbl);

    g_free(glib_version_info);
    g_free(gtk_version_info);
}

static int app_init(void) {
    sysobj_init(NULL);
    uri_set_function((uri_handler)uri_sysobj);
    about_init();
    sysobj_virt_add_simple_mkpath(":app/watchlist/no-group", NULL, "*", VSO_TYPE_DIR);
    class_add_simple(":app/watchlist*", "Watchlist item", "watchlist", OF_GLOB_PATTERN, 1.0);
    return 1;
}

static void app_cleanup(void) {
    sysobj_cleanup();
}

static void notebook_goto_page(const gchar *name, int page_num) {
    if (notebook)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), PAGE_BROWSER);
    else if (stack)
        gtk_stack_set_visible_child_name(GTK_STACK(stack), name);
}

static void notebook_add_page(const gchar *name, const gchar *label, GtkWidget *notebook, GtkWidget *page_widget, gint border) {
    if (notebook) {
        GtkWidget *lbl = gtk_label_new (label);
        gtk_container_set_border_width (GTK_CONTAINER (page_widget), border);
        gtk_widget_set_size_request (page_widget, 100, 75);
        gtk_notebook_append_page(GTK_NOTEBOOK (notebook), page_widget, lbl);
        gtk_widget_show (page_widget);
    } else if (stack) {
        gtk_stack_add_titled(GTK_STACK(stack), page_widget, name, label);
        gtk_widget_show(page_widget);
    }
}

void watchlist_clear() {
    sysobj_virt_remove(":app/watchlist/*");
}

void watchlist_add(const gchar *path) {
    gchar *name = g_path_get_basename(path);
    sysobj_virt_add_simple(":app/watchlist", name, path,
        VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_AUTOLINK );
}

void browser_navigate(const gchar *new_location) {
    bp_sysobj_browser_navigate(BP_SYSOBJ_BROWSER(browser), new_location);
    notebook_goto_page("browser", PAGE_BROWSER);
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
    app_cleanup();
    gtk_main_quit();
}

void search_activated(gpointer view, gchar *sysobj_path, gpointer user_data) {
    browser_navigate(sysobj_path);
    g_free(sysobj_path);
}

void watchlist_activated(gpointer view, gchar *sysobj_path, gpointer user_data) {
    /* resolve the symlink target */
    sysobj *obj = sysobj_new_from_fn(sysobj_path, NULL);
    gchar *target = g_strdup(obj->path);
    sysobj_free(obj);
    g_free(sysobj_path);
    browser_navigate(target);
    g_free(target);
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

    //if (DEBUG_BUILD) class_dump_list();

    browser = bp_sysobj_browser_new();
    watchlist = bp_sysobj_view_new();
    bp_sysobj_view_set_include_target(BP_SYSOBJ_VIEW(watchlist), FALSE);
    bp_sysobj_view_set_max_depth(BP_SYSOBJ_VIEW(watchlist), 1);
    bp_sysobj_view_set_path(BP_SYSOBJ_VIEW(watchlist), ":app/watchlist");
    g_signal_connect(watchlist, "item-activated",
          G_CALLBACK(watchlist_activated), watchlist);
    search = bp_sysobj_search_new();
    g_signal_connect(search, "search-result-activated",
          G_CALLBACK(search_activated), search);

    /* notebook pages */
    GtkWidget *blah = NULL;
    if (USE_NOTEBOOK) {
        blah = notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    } else {
        blah = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        stack = gtk_stack_new();
        GtkWidget *switcher = gtk_stack_switcher_new();
        gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
        gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(blah), switcher, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(blah), stack, TRUE, TRUE, 0);
        gtk_widget_show(switcher);
        gtk_widget_show(stack);
    }
    notebook_add_page("browser", _("Browser"), notebook, browser, 5);
    notebook_add_page("search", _("Search"), notebook, search, 5);
    notebook_add_page("watchlist", _("Watchlist"), notebook, watchlist, 5);
    notebook_add_page("about", _("About"), notebook, about, 5);

    /* set up main window */
    app_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (app_window, "delete-event",
          G_CALLBACK (delete_event), NULL);
    g_signal_connect (app_window, "destroy",
          G_CALLBACK (destroy), NULL);

    gtk_container_add(GTK_CONTAINER(app_window), blah);
    gtk_widget_show(blah);

    GdkPixbuf *icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "face-monkey", 128, 0, NULL);
    if (icon) {
        gtk_window_set_icon(GTK_WINDOW(app_window), icon);
        g_object_unref(icon);
    }
    gtk_container_set_border_width(GTK_CONTAINER(app_window), 5);
    gtk_window_resize (GTK_WINDOW (app_window), 740, 400);

    /* get ready */
    if (query)
        browser_navigate(query);
    else
        browser_navigate(":");

    /* go */
    gtk_widget_show (app_window);
    gtk_main ();

    return 0;
}

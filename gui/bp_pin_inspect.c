
#include "bp_pin_inspect.h"
#include "sysobj_foreach.h"
#include "sysobj_extras.h"
#include "format_funcs.h"
#include <inttypes.h>

#include "gtk_backports.c"

/* Forward declarations */
static void _create(bpPinInspect *s);
static void _cleanup(bpPinInspect *s);

/* ... captured signals */
static gboolean _activate_link (GtkLabel *label, gchar *uri, gpointer user_data);

/* Private class member */
#define BP_PIN_INSPECT_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    BP_PIN_INSPECT_TYPE, bpPinInspectPrivate))

typedef struct _bpPinInspectPrivate bpPinInspectPrivate;

struct _bpPinInspectPrivate {
    const pin *p;
    int fmt_opts;

    GSList *wlh;

    GtkTextBuffer *val_formatted;
    GtkTextBuffer *val_raw;

    GtkWidget *lbl_top;
    GtkWidget *lbl_vendor;
    GtkWidget *lbl_wlh; /* what links here */
    GtkWidget *lbl_debug;

    GtkWidget *value_notebook;
    GtkWidget *raw_view;
    GtkWidget *help_container;
    GtkWidget *lbl_help;
    GtkWidget *vendor_container; /* needed to change the label of the notebook tab */
};

G_DEFINE_TYPE(bpPinInspect, bp_pin_inspect, GTK_TYPE_PANED);

/* Initialize the bpPinInspect class */
static void
bp_pin_inspect_class_init(bpPinInspectClass *klass)
{
    /* TODO: g_type_class_add_private deprecated in glib 2.58 */

    /* Add private indirection member */
    g_type_class_add_private(klass, sizeof(bpPinInspectPrivate));
}

/* Initialize a new bpPinInspect instance */
static void
bp_pin_inspect_init(bpPinInspect *s)
{
    /* This means that bpPinInspect doesn't supply its own GdkWindow */
    gtk_widget_set_has_window(GTK_WIDGET(s), FALSE);
    /* Set redraw on allocate to FALSE if the top left corner of your widget
     * doesn't change when it's resized; this saves time */
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(s), FALSE);

    /* Initialize private members */
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);
    memset(priv, sizeof(bpPinInspectPrivate), 0);

    _create(s);

    g_signal_connect(s, "destroy", G_CALLBACK(_cleanup), NULL);
}

/* Return a new bpPinInspect cast to a GtkWidget */
GtkWidget *
bp_pin_inspect_new()
{
    return GTK_WIDGET(g_object_new(bp_pin_inspect_get_type(), NULL));
}

static gboolean _activate_link (GtkLabel *label, gchar *uri, gpointer user_data) {
    return uri_open(uri);
}

void _wrap_toggled(GtkToggleButton *togglebutton, GtkWidget *target) {
    if(GTK_IS_TEXT_VIEW(target)) {
        gboolean wrap = gtk_toggle_button_get_active(togglebutton);
        if (wrap)
            gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(target), GTK_WRAP_WORD_CHAR);
        else
            gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(target), GTK_WRAP_NONE);
    }
}

#define CLIP_COPY_ALL 0
void _copy_to_clipboard(GtkButton *button, GtkWidget *target) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    if(GTK_IS_TEXT_VIEW(target)) {
        GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(target));
        GtkTextIter start, end, old_start, old_end;
        gboolean sel = gtk_text_buffer_get_has_selection(buffer);
        if (sel && !CLIP_COPY_ALL) {
            gtk_text_buffer_copy_clipboard(buffer, clipboard);
        } else {
            gtk_text_buffer_get_selection_bounds(buffer, &old_start, &old_end);
            gtk_text_buffer_get_bounds(buffer, &start, &end);
            gtk_text_buffer_select_range(buffer, &start, &end);
            gtk_text_buffer_copy_clipboard(buffer, clipboard);
            gtk_text_buffer_select_range(buffer, &old_start, &old_end);
        }
    }
    if(GTK_IS_LABEL(target)) {
        gtk_clipboard_set_text(clipboard, gtk_label_get_text(GTK_LABEL(target)), -1);
    }
}

static void _cleanup(bpPinInspect *s) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);
}

typedef struct {
    bpPinInspectPrivate *priv;
    gchar *target;
    GThread *thread;
    gboolean valid;
    GSList *links;
} wlh_state;

void wlh_mt(wlh_state *s, gboolean done) {
    if (!s || !s->valid) return;
    gchar *lmt = NULL;
    for(GSList *l = s->links; l; l = l->next) {
        lmt = appfs(lmt, "\n", "<a href=\"sysobj:%s\">%s</a>", (gchar*)l->data, (gchar*)l->data);
    }
    if (!done) {
        lmt = appfs(lmt, "\n", "...");
    }
    gtk_label_set_markup(GTK_LABEL(s->priv->lbl_wlh), lmt ? lmt : "");
    g_free(lmt);
}

gboolean wlh_examine(sysobj *obj, wlh_state *s, const sysobj_foreach_stats *stats) {
    bpPinInspectPrivate *priv = s ? s->priv : NULL;
    if (priv && s->valid && priv->p && priv->p->obj) {
        if (!SEQ(s->target, priv->p->obj->path)) {
            s->valid = FALSE;
            return SYSOBJ_FOREACH_STOP;
        }
        if (obj->req_is_link && SEQ(obj->path, s->target)) {
            s->links = g_slist_append(s->links, g_strdup(obj->path_req));
            wlh_mt(s, FALSE);
        }
        return SYSOBJ_FOREACH_CONTINUE;
    }
    s->valid = FALSE;
    return SYSOBJ_FOREACH_STOP;
}

static gpointer wlh_thread_main(wlh_state *s) {
    wlh_mt(s, FALSE);
    sysobj_foreach(NULL, (f_sysobj_foreach)wlh_examine, s, SO_FOREACH_MT);
    wlh_mt(s, TRUE);
    g_thread_unref(s->thread); // self, what implications?
    g_free(s->target);
    g_slist_free_full(s->links, (GDestroyNotify)g_free);
    g_free(s);
}

void wlh_start(bpPinInspectPrivate *priv) {
    if (priv && priv->p && priv->p->obj) {
        wlh_state *s = g_new0(wlh_state, 1);
        s->target = g_strdup(priv->p->obj->path);
        s->priv = priv;
        s->valid = TRUE;
        s->thread = g_thread_new(NULL, (GThreadFunc)wlh_thread_main, s);
    }
}

void _scan_wlh(GtkButton *button, bpPinInspectPrivate *priv) {
    wlh_start(priv);
}

static GtkWidget *notebook_add_page(const gchar *name, const gchar *label, GtkWidget *notebook, GtkWidget *page_widget, gint border, bpPinInspectPrivate *priv) {
    GtkWidget *lbl = gtk_label_new (label);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *actions = gtk_action_bar_new();

    gtk_widget_set_size_request(scroll, 100, 100);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(scroll), page_widget);
    gtk_widget_show(page_widget);
    gtk_widget_show(scroll);

    if(GTK_IS_TEXT_VIEW(page_widget)) {
        GtkWidget *btn_wrap = gtk_check_button_new_with_label(_("wrap text"));
        GtkWrapMode wm = gtk_text_view_get_wrap_mode(GTK_TEXT_VIEW(page_widget));
        if (wm == GTK_WRAP_NONE)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_wrap), FALSE);
        else
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_wrap), TRUE);
        g_signal_connect(btn_wrap, "toggled", G_CALLBACK(_wrap_toggled), page_widget);
        gtk_widget_show(btn_wrap);
        gtk_action_bar_pack_start(GTK_ACTION_BAR(actions), btn_wrap);
    }

    if(SEQ(name, "wlh") ) {
        GtkWidget *btn_scan = gtk_button_new_from_icon_name("view-refresh", GTK_ICON_SIZE_LARGE_TOOLBAR);
        gtk_widget_set_tooltip_text(btn_scan, _("Scan"));
        g_signal_connect(btn_scan, "clicked", G_CALLBACK(_scan_wlh), priv);
        gtk_widget_show(btn_scan);
        gtk_action_bar_pack_start(GTK_ACTION_BAR(actions), btn_scan);
    }

    GtkWidget *btn_copy = gtk_button_new_from_icon_name("edit-copy", GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_set_tooltip_text(btn_copy, _("Copy to clipboard"));
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(_copy_to_clipboard), page_widget);
    gtk_widget_show(btn_copy);
    gtk_action_bar_pack_start(GTK_ACTION_BAR(actions), btn_copy);

    gtk_widget_show(actions);

    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), actions, FALSE, FALSE, 0);
    gtk_widget_show(box);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, lbl);
    return box;
}

static void _create(bpPinInspect *s) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(s), GTK_ORIENTATION_VERTICAL);

    GtkWidget *lbl_top = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_top), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_top), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_top, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_top, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_top, 10);
    //gtk_label_set_selectable(GTK_LABEL(lbl_top), TRUE);
    g_signal_connect(lbl_top, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_top);

    GtkWidget *lbl_vendor = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_vendor), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_vendor), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_vendor, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_vendor, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_vendor, 10);
    //gtk_label_set_selectable(GTK_LABEL(lbl_vendor), TRUE);
    g_signal_connect (lbl_vendor, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_vendor);

    GtkWidget *lbl_debug = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_debug), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_debug), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_debug, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_debug, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_debug, 10);
    //gtk_label_set_selectable(GTK_LABEL(lbl_debug), TRUE);
    g_signal_connect (lbl_debug, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_debug);

    GtkWidget *lbl_wlh = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_wlh), FALSE);
    gtk_label_set_justify(GTK_LABEL(lbl_wlh), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_wlh, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_wlh, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_wlh, 10);
    g_signal_connect (lbl_wlh, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_wlh);

    GtkTextBuffer *val_formatted = gtk_text_buffer_new(NULL);
    GtkWidget *text_formatted = gtk_text_view_new_with_buffer(val_formatted);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_formatted), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_formatted), GTK_WRAP_WORD_CHAR);
    gtk_widget_show(text_formatted);

    GtkTextBuffer *val_raw = gtk_text_buffer_new(NULL);
    GtkWidget *text_raw = gtk_text_view_new_with_buffer(val_raw);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_raw), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_raw), GTK_WRAP_NONE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_raw), TRUE);
    gtk_widget_show(text_raw);

    GtkWidget *value_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (value_notebook), GTK_POS_TOP);
    notebook_add_page("formatted", _("Formatted"), value_notebook, text_formatted, 5, priv);
    notebook_add_page("raw", _("Raw"), value_notebook, text_raw, 5, priv);
    GtkWidget *vendor_container =
        notebook_add_page("vendor", _("Vendors"), value_notebook, lbl_vendor, 5, priv);
    notebook_add_page("wlh", _("Links"), value_notebook, lbl_wlh, 5, priv);
    notebook_add_page("debug", _("Debug"), value_notebook, lbl_debug, 5, priv);
    gtk_widget_show(value_notebook);

    GtkWidget *box_sections = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box_sections), lbl_top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_sections), value_notebook, TRUE, TRUE, 0);

    GtkWidget *lbl_help = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_help), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_help), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_help, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_help, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_help, 10);
    gtk_label_set_selectable(GTK_LABEL(lbl_help), TRUE);
    g_signal_connect(lbl_help, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_help);

    GtkWidget *help_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(help_scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(help_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(help_scroll), lbl_help);
    gtk_widget_show(help_scroll);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_position(GTK_PANED(s), 200);
    gtk_paned_pack1(GTK_PANED(s), box_sections, TRUE, FALSE); gtk_widget_show (box_sections);
    gtk_paned_pack2(GTK_PANED(s), help_scroll, FALSE, FALSE); gtk_widget_show (help_scroll);

    priv->val_formatted = val_formatted;
    priv->val_raw = val_raw;
    priv->raw_view = text_raw;
    priv->lbl_top = lbl_top;
    priv->lbl_vendor = lbl_vendor;
    priv->lbl_wlh = lbl_wlh;
    priv->lbl_debug = lbl_debug;
    priv->help_container = help_scroll;
    priv->lbl_help = lbl_help;
    priv->value_notebook = value_notebook;
    priv->vendor_container = vendor_container;
}

static gchar *print_hex(const uint8_t* bytes, gsize len) {
    gchar *ret = NULL;

    const char *fmt = ngettext("%lu byte", "%lu bytes", (unsigned long)len);
    gchar *szstr = g_strdup_printf(fmt, (unsigned long)len);

    if (bytes && len) {
        ret = g_strdup_printf("%s:", szstr);
        gchar digits[26] = "";
        gchar chars[12] = "";
        gsize i = 0;
        for(; i < len; i++) {
            int dl = i%8;
            char c = bytes[i];
            if (!isprint(c)) c = '.';
            snprintf(digits + dl*3, 4, "%02X ", (unsigned int)bytes[i]);
            snprintf(chars + dl*1, 4, "%c ", (unsigned int)c);
            if (dl == 7) {
                ret = appfs(ret, "\n", "%04x: %s %s", (unsigned int)((i/8) * 8), digits, chars);
                strcpy(digits, "");
                strcpy(chars, "");
            }
        }
        for(; i%8; i++) {
            int dl = i%8;
            snprintf(digits + dl*3, 4, "   ");
            snprintf(chars + dl*1, 4, "  ");
            if (dl == 7)
                ret = appfs(ret, "\n", "%04x: %s %s", (unsigned int)(((i-1)/8) * 8), digits, chars);
        }
    }

    if (!ret)
        return szstr;
    g_free(szstr);
    return ret;
}

void bp_pin_inspect_do(bpPinInspect *s, const pin *p, int fmt_opts) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);

    gboolean is_new = FALSE;
    if (priv->p != p)
        is_new = TRUE;

    priv->p = p;
    priv->fmt_opts = fmt_opts;
    if (!p) {
        gtk_label_set_markup(GTK_LABEL(priv->lbl_top), "");
        gtk_label_set_markup(GTK_LABEL(priv->lbl_vendor), "");
        gtk_label_set_markup(GTK_LABEL(priv->lbl_debug), "");
        gtk_widget_hide(priv->help_container);
        gtk_text_buffer_set_text(priv->val_formatted, "", -1);
        gtk_text_buffer_set_text(priv->val_raw, "", -1);
        return;
    }

    /* item */
    gchar *fperm = NULL;
    if (!p->obj->data.is_dir) {
        fperm = g_strdup_printf("<small>(%s%s/%s%s)</small>",
            p->obj->root_can_read ? "r" : "-",
            p->obj->root_can_write ? "w" : "-",
            p->obj->others_can_read ? "r" : "-",
            p->obj->others_can_write ? "w" : "-"
        );
    } else
        fperm = g_strdup("");

    gchar *label = g_strdup(sysobj_label(p->obj));
    gchar *halp = g_strdup(sysobj_halp(p->obj));
    gchar *nice = sysobj_format(p->obj, fmt_opts | FMT_OPT_COMPLETE);

    gchar *tag = NULL;
    if (p->obj->cls) {
        if (p->obj->cls->tag)
            tag = g_strdup_printf("{<a href=\"sysobj::sysobj/classes/%s\">%s</a>}", p->obj->cls->tag, p->obj->cls->tag);
        else
            tag = g_strdup_printf("{%s}", p->obj->cls->pattern);
    } else
        tag = g_strdup("{none}");

    const gchar *suggest_path = sysobj_suggest(p->obj);
    gchar *suggest = NULL;
    if (suggest_path)
        suggest = g_strdup_printf(_("Consider using <a href=\"sysobj:%s\">%s</a>"), suggest_path, suggest_path);

    /* debug stuff */
    gchar *data_info = g_strdup_printf("was_read = %s; is_null = %s; len = %lu byte(s)%s; guess_nbase = %d",
        p->obj->data.was_read ? "yes" : "no", (p->obj->data.any == NULL) ? "yes" : "no",
        p->obj->data.len, p->obj->data.is_utf8 ? ", utf8" : "", p->obj->data.maybe_num);
    double ui = sysobj_update_interval(p->obj);
    gchar *uidesc = "";
    if (ui == UPDATE_INTERVAL_DEFAULT) uidesc = " (use-default)";
    if (ui == UPDATE_INTERVAL_UNSPECIFIED) uidesc = " (unspecified)";
    if (ui == UPDATE_INTERVAL_NEVER) uidesc = " (never)";
    gchar *update = g_strdup_printf("update_interval = %0.2lfs%s, last_update = %0.4lfs", ui, uidesc, p->obj->data.stamp);
    gchar *pin_info = g_strdup_printf("hist_stat = %d, hist_len = %" PRIu64 "/%" PRIu64 ", hist_mem_size = %" PRIu64 " (%" PRIu64 " bytes)",
        p->history_status, p->history_len, p->history_max_len, p->history_mem, p->history_mem * sizeof(sysobj_data) );
    gchar *fmt = fmt_opts_str(fmt_opts);
    gchar *oflags = flags_str(sysobj_flags(p->obj));

    if (is_new) {
        gchar *ven_mt = NULL;
        int vcnt = 0;
        GSList *l = NULL, *vl = sysobj_vendors(p->obj);
        vl = vendor_list_remove_duplicates_deep(vl);
        if (vl) {
            for(l = vl; l; l = l->next) {
                const Vendor *v = l->data;
                if (!v) continue;
                gchar *full_link = NULL;
                gchar *ven_tag = v->name_short ? g_strdup(v->name_short) : g_strdup(v->name);
                tag_vendor(&ven_tag, 0, ven_tag, v->ansi_color, fmt_opts);
                //ven_mt = appfs(ven_mt, "\n", "%p", v);
                if (v->name_short)
                    ven_mt = appfs(ven_mt, "\n", "%s", v->name);
                ven_mt = appfs(ven_mt, "\n", "%s", ven_tag);
                if (v->url) {
                    if (!g_str_has_prefix(v->url, "http") )
                        full_link = g_strdup_printf("http://%s", v->url);
                    ven_mt = appfs(ven_mt, "\n", "<b>%s:</b> <a href=\"%s\">%s</a>", _("URL"), full_link ? full_link : v->url, v->url);
                    g_free(full_link);
                    full_link = NULL;
                }
                if (v->url_support) {
                    if (!g_str_has_prefix(v->url_support, "http") )
                        full_link = g_strdup_printf("http://%s", v->url_support);
                    ven_mt = appfs(ven_mt, "\n", "<b>%s:</b> <a href=\"%s\">%s</a>", _("Support URL"), full_link ? full_link : v->url_support, v->url_support);
                    g_free(full_link);
                    full_link = NULL;
                }
                if (v->wikipedia) {
                    /* sending the title to wikipedia.com/wiki will autmatically handle the language and section parts,
                     * but perhaps that shouldn't be relied on so much? */
                    full_link = g_strdup_printf("http://wikipedia.com/wiki/%s", v->wikipedia);
                    for(gchar *p = full_link; *p; p++) {
                        if (*p == ' ') *p = '_';
                    }
                    ven_mt = appfs(ven_mt, "\n", "<b>%s:</b> <a href=\"%s\">%s</a>", _("Wikipedia"), full_link ? full_link : v->wikipedia, v->wikipedia);
                    g_free(full_link);
                    full_link = NULL;
                }
                g_free(ven_tag);
                ven_mt = appfs(ven_mt, "\n", " ");
                vcnt++;
            }
            vendor_list_free(vl);
        }
        gtk_label_set_markup(GTK_LABEL(priv->lbl_vendor), ven_mt ? ven_mt : "");

        /* show the count in the tab label */
        const gchar *lbl_fmt = ngettext("Vendor (%d)", "Vendors (%d)", vcnt);
        gchar *lbl_txt = g_strdup_printf(lbl_fmt, vcnt);
        gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(priv->value_notebook), priv->vendor_container, lbl_txt);
        g_free(lbl_txt);
    }

    if (!label)
        label = g_strdup("");

    gchar *mt = NULL;
    if (is_new) {
        mt = g_strdup_printf(
            /* name   */ "<big><big>%s</big></big> %s\n"
            /* resolv */ "<a href=\"sysobj:%s\">%s</a>\n"
            /* suggest */ "%s%s"
            /* label  */ "%s\n",
                p->obj->name, fperm,
                p->obj->path, p->obj->path,
                suggest ? suggest : "",
                suggest ? "\n" : "",
                label );
        gtk_label_set_markup(GTK_LABEL(priv->lbl_top), mt);
    }

    GtkTextIter iter;
    gtk_text_buffer_set_text(priv->val_formatted, "", -1);
    gtk_text_buffer_get_iter_at_offset(priv->val_formatted, &iter, 0);
    gtk_text_buffer_insert_markup(priv->val_formatted, &iter, nice, -1);

    if (p->obj->data.is_utf8)
        gtk_text_buffer_set_text(priv->val_raw, p->obj->data.str, -1);
    else {
        gchar *hex = print_hex(p->obj->data.uint8, p->obj->data.len);
        gtk_text_buffer_set_text(priv->val_raw, hex ? hex : "", -1);
        g_free(hex);
    }

    mt = g_strdup_printf("debug info:\n"
        /* fmt_opts */  "fmt: %s\n"
        /* flags */     "flags%s: %s\n"
        /* data info */ "%s\n"
        /* pin info */  "%s\n"
        /* update */    "%s\n",
        fmt, tag, oflags, data_info, pin_info, update);
    gtk_label_set_markup(GTK_LABEL(priv->lbl_debug), mt);

    if (is_new) {
        if (halp) {
            gtk_label_set_markup(GTK_LABEL(priv->lbl_help), halp );
            gtk_widget_show(priv->help_container);
        } else {
            gtk_widget_hide(priv->help_container);
        }

        gtk_label_set_markup(GTK_LABEL(priv->lbl_wlh), _("Press \"Scan\" below to search for incoming links."));
    }

    g_free(fperm);
    g_free(suggest);
    g_free(label);
    g_free(nice);
    g_free(tag);
    g_free(update);
    g_free(data_info);
    g_free(pin_info);
    g_free(fmt);
}

const pin *bp_pin_inspect_get_pin(bpPinInspect *s) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);
    return priv->p;
}


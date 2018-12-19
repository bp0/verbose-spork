
#include "bp_pin_inspect.h"
#include "sysobj_extras.h"
#include <inttypes.h>

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

    GtkTextBuffer *val_formatted;
    GtkTextBuffer *val_raw;

    GtkWidget *lbl_top;
    GtkWidget *lbl_vendor;
    GtkWidget *lbl_debug;

    GtkWidget *help_container;
    GtkWidget *lbl_help;
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

static void _cleanup(bpPinInspect *s) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);
}

static void notebook_add_page(const gchar *name, const gchar *label, GtkWidget *notebook, GtkWidget *page_widget, gint border) {
    GtkWidget *lbl = gtk_label_new (label);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 100, 100);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(scroll), page_widget);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll, lbl);
    gtk_widget_show(page_widget);
    gtk_widget_show(scroll);
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

    GtkTextBuffer *val_formatted = gtk_text_buffer_new(NULL);
    GtkWidget *text_formatted = gtk_text_view_new_with_buffer(val_formatted);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_formatted), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_formatted), GTK_WRAP_WORD_CHAR);
    gtk_widget_show(text_formatted);

    GtkTextBuffer *val_raw = gtk_text_buffer_new(NULL);
    GtkWidget *text_raw = gtk_text_view_new_with_buffer(val_raw);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_raw), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_raw), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_raw), TRUE);
    gtk_widget_show(text_raw);

    GtkWidget *value_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (value_notebook), GTK_POS_TOP);
    notebook_add_page("formatted", _("Formatted"), value_notebook, text_formatted, 5);
    notebook_add_page("raw", _("Raw"), value_notebook, text_raw, 5);
    notebook_add_page("vendor", _("Vendor"), value_notebook, lbl_vendor, 5);
    notebook_add_page("debug", _("Debug"), value_notebook, lbl_debug, 5);
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
    priv->lbl_top = lbl_top;
    priv->lbl_vendor = lbl_vendor;
    priv->lbl_debug = lbl_debug;
    priv->help_container = help_scroll;
    priv->lbl_help = lbl_help;
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
        gtk_label_set_markup(GTK_LABEL(priv->lbl_debug), "");
        gtk_widget_hide(priv->help_container);
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
    gchar *data_info = g_strdup_printf("was_read = %s; is_null = %s; len = %lu byte(s)%s; guess_nbase = %d\ntag = %s",
        p->obj->data.was_read ? "yes" : "no", (p->obj->data.any == NULL) ? "yes" : "no",
        p->obj->data.len, p->obj->data.is_utf8 ? ", utf8" : "", p->obj->data.maybe_num, tag);
    gchar *update = g_strdup_printf("update_interval = %0.4lfs, last_update = %0.4lfs", p->update_interval, p->obj->data.stamp);
    gchar *pin_info = g_strdup_printf("hist_stat = %d, hist_len = %" PRIu64 "/%" PRIu64 ", hist_mem_size = %" PRIu64 " (%" PRIu64 " bytes)",
        p->history_status, p->history_len, p->history_max_len, p->history_mem, p->history_mem * sizeof(sysobj_data) );

    if (is_new) {
        gchar *ven_mt = NULL;
        GSList *l = NULL, *vl = sysobj_vendors(p->obj);
        vl = vendor_list_remove_duplicates(vl);
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
                    ven_mt = appfs(ven_mt, "\n", "<b>URL:</b> <a href=\"%s\">%s</a>", full_link ? full_link : v->url, v->url);
                    g_free(full_link);
                    full_link = NULL;
                }
                if (v->url_support) {
                    if (!g_str_has_prefix(v->url_support, "http") )
                        full_link = g_strdup_printf("http://%s", v->url_support);
                    ven_mt = appfs(ven_mt, "\n", "<b>Support URL:</b> <a href=\"%s\">%s</a>", full_link ? full_link : v->url_support, v->url_support);
                    g_free(full_link);
                    full_link = NULL;
                }
                g_free(ven_tag);
                ven_mt = appfs(ven_mt, "\n", " ");
            }
            vendor_list_free(vl);
        }
        gtk_label_set_markup(GTK_LABEL(priv->lbl_vendor), ven_mt ? ven_mt : "");
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
        gchar *hex = NULL;
        gsize i = 0, l = p->obj->data.len;
        uint8_t *byte = (uint8_t*)p->obj->data.any;

        if (l)
            hex = g_strdup_printf("%lu bytes:\n%02X", p->obj->data.len, byte[0]);
        else
            hex = g_strdup_printf("%lu bytes:\n", p->obj->data.len);
        for(i = 1; i < l; i++)
            hex = appf(hex, "%02X", byte[i]);
        gtk_text_buffer_set_text(priv->val_raw, hex ? hex : "", -1);
        g_free(hex);
    }

    mt = g_strdup_printf("debug info:\n"
        /* data info */ "%s\n"
        /* pin info */ "%s\n"
        /* update */ "%s\n",
        data_info, pin_info, update);
    gtk_label_set_markup(GTK_LABEL(priv->lbl_debug), mt);

    if (is_new) {
        if (halp) {
            gtk_label_set_markup(GTK_LABEL(priv->lbl_help), halp );
            gtk_widget_show(priv->help_container);
        } else {
            gtk_widget_hide(priv->help_container);
        }
    }

    g_free(fperm);
    g_free(suggest);
    g_free(label);
    g_free(nice);
    g_free(tag);
    g_free(update);
    g_free(data_info);
    g_free(pin_info);
}

const pin *bp_pin_inspect_get_pin(bpPinInspect *s) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);
    return priv->p;
}

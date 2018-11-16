
#include "bp_pin_inspect.h"
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
    GtkWidget *container;
    GtkWidget *box_sections;
    GtkWidget *lbl_top;
    GtkWidget *lbl_value;
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

static void _create(bpPinInspect *s) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);

    gtk_orientable_set_orientation(GTK_ORIENTABLE(s), GTK_ORIENTATION_VERTICAL);

    GtkWidget *lbl_top = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_top), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_top), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_top, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_top, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_top, 10);
    g_signal_connect(lbl_top, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_top);

    GtkWidget *lbl_value = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_value), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_value), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_value, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_value, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_value, 10);
    g_signal_connect (lbl_value, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_value);

    GtkWidget *lbl_debug = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_debug), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_debug), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_debug, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_debug, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_debug, 10);
    g_signal_connect (lbl_debug, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_debug);

    GtkWidget *box_sections = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box_sections), lbl_top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_sections), lbl_value, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_sections), lbl_debug, FALSE, FALSE, 0);

    GtkWidget *top_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(top_scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(top_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(top_scroll), box_sections);
    gtk_widget_show(box_sections);

    GtkWidget *lbl_help = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_help), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_help), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(lbl_help, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl_help, GTK_ALIGN_START);
    gtk_widget_set_margin_start(lbl_help, 10);
    g_signal_connect(lbl_help, "activate-link", G_CALLBACK(_activate_link), NULL);
    gtk_widget_show(lbl_help);

    GtkWidget *help_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(help_scroll), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(help_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(help_scroll), lbl_help);
    gtk_widget_show(help_scroll);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_position(GTK_PANED(s), 200);
    gtk_paned_pack1(GTK_PANED(s), top_scroll, TRUE, FALSE); gtk_widget_show (top_scroll);
    gtk_paned_pack2(GTK_PANED(s), help_scroll, FALSE, FALSE); gtk_widget_show (help_scroll);

    priv->box_sections = box_sections;
    priv->lbl_top = lbl_top;
    priv->lbl_value = lbl_value;
    priv->lbl_debug = lbl_debug;
    priv->help_container = help_scroll;
    priv->lbl_help = lbl_help;
}

void bp_pin_inspect_do(bpPinInspect *s, const pin *p, int fmt_opts) {
    bpPinInspectPrivate *priv = BP_PIN_INSPECT_PRIVATE(s);

    gboolean is_new = FALSE;
    if (!p) return;
    if (priv->p != p)
        is_new = TRUE;

    priv->p = p;
    priv->fmt_opts = fmt_opts;

    /* item */
    gchar *label = g_strdup(sysobj_label(p->obj));
    gchar *halp = g_strdup(sysobj_halp(p->obj));
    gchar *nice = sysobj_format(p->obj, fmt_opts);
    gchar *tag = g_strdup_printf("{%s}", p->obj->cls ? (p->obj->cls->tag ? p->obj->cls->tag : p->obj->cls->pattern) : "none");
    const gchar *suggest_path = sysobj_suggest(p->obj);
    gchar *suggest = NULL;
    if (suggest_path)
        suggest = g_strdup_printf(_("Consider using <a href=\"sysobj:%s\">%s</a>"), suggest_path, suggest_path);

    /* debug stuff */
    gchar *data_info = g_strdup_printf("raw_size = %lu byte(s)%s; guess_nbase = %d\ntag = %s",
        p->obj->data.len, p->obj->data.is_utf8 ? ", utf8" : "", p->obj->data.maybe_num, tag);
    gchar *update = g_strdup_printf("update_interval = %0.4lfs, last_update = %0.4lfs", p->update_interval, p->last_update);
    gchar *pin_info = g_strdup_printf("hist_stat = %d, hist_len = %" PRIu64 "/%" PRIu64 ", hist_mem_size = %" PRIu64 " (%" PRIu64 " bytes)",
        p->history_status, p->history_len, p->history_max_len, p->history_mem, p->history_mem * sizeof(sysobj_data) );

    if (!label)
        label = g_strdup("");

    gchar *mt = NULL;
    if (is_new) {
        mt = g_strdup_printf(
            /* name   */ "<big><big>%s</big></big>\n"
            /* resolv */ "<a href=\"sysobj:%s\">%s</a>\n"
            /* suggest */ "%s%s"
            /* label  */ "%s\n",
                p->obj->name,
                p->obj->path, p->obj->path,
                suggest ? suggest : "",
                suggest ? "\n" : "",
                label );
        gtk_label_set_markup(GTK_LABEL(priv->lbl_top), mt);
    }

    mt = g_strdup_printf(/* value  */ "%s\n", nice);
    gtk_label_set_markup(GTK_LABEL(priv->lbl_value), mt);

    mt = g_strdup_printf("debug info:\n"
        /* data info */ "%s\n"
        /* pin info */ "%s\n"
        /* update */ "%s",
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

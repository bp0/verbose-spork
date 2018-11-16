
#include <stdio.h>
#include "uri_handler.h"

static uri_handler uri_func = NULL;

void uri_set_function(uri_handler f) {
    uri_func = f;
}

gboolean uri_open(const gchar *uri) {
    gboolean ret = FALSE;
    if (uri_func)
        ret = uri_func(uri);
    if (ret) return TRUE;

    return uri_open_default(uri);
}

gboolean uri_open_default(const gchar *uri) {
    gchar *argv[] = { "/usr/bin/xdg-open", (gchar*)uri, NULL };
    GError *err = NULL;
    gboolean r = g_spawn_async(NULL, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, &err );
    if (err) {
        fprintf(stderr, "Error opening URI %s: %s\n", uri, err->message);
        g_error_free(err);
    }
    return TRUE;
}

#ifndef URI_HANDLER_H
#define URI_HANDLER_H

#include <glib.h>

typedef gboolean (*uri_handler)(const gchar *uri);

void uri_set_function(uri_handler f);
gboolean uri_open(const gchar *uri);
gboolean uri_open_default(const gchar *uri); /* uses xdg-open */

#endif

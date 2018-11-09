
#ifndef _UTIL_USB_H_
#define _UTIL_USB_H_

#include <glib.h>
#include <stdio.h>
#include <stdint.h>  /* for *int*_t types */

typedef struct {
    uint32_t vendor;
    uint32_t device;

    gchar *address;
    gchar *sort_key;
    gchar *vendor_str;
    gchar *device_str;
} util_usb_id;

void util_usb_id_free(util_usb_id *s);
gboolean util_usb_ids_lookup(util_usb_id *usbd);
int util_usb_ids_lookup_list(GSList *items); /* found count or -1 for error */

#endif

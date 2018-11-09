
#ifndef _UTIL_PCI_H_
#define _UTIL_PCI_H_

#include <glib.h>
#include <stdio.h>
#include <stdint.h>  /* for *int*_t types */

typedef struct {
    uint32_t vendor;
    uint32_t device;
    uint32_t sub_vendor;
    uint32_t sub_device;
    uint32_t dev_class;

    gchar *address;
    gchar *sort_key;
    gchar *vendor_str;
    gchar *device_str;
    gchar *sub_vendor_str;
    gchar *sub_device_str;
    gchar *dev_class_str;
} util_pci_id;

void util_pci_id_free(util_pci_id *s);
gboolean util_pci_ids_lookup(util_pci_id *pcid);
gboolean util_pci_ids_lookup_list(GSList *items);

#endif

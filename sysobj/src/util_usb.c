
#include "util_usb.h"

void util_usb_id_free(util_usb_id *s) {
    if (s) {
        g_free(s->address);
        g_free(s->sort_key);
        g_free(s->vendor_str);
        g_free(s->device_str);
        g_free(s);
    }
}

gboolean util_usb_ids_lookup(util_usb_id *usbd) {
    GSList *tmp = NULL;
    tmp = g_slist_append(tmp, usbd);
    gboolean ret = util_usb_ids_lookup_list(tmp);
    g_slist_free(tmp);
    return ret;
}

static int cmp_util_usb_id(const gpointer a, const gpointer b) {
    util_usb_id *A = a ? a : NULL;
    util_usb_id *B = b ? b : NULL;
    if (!A) return (B ? -1 : 0);
    if (!B) return (A ? 1 : 0);
    if (!A->sort_key)
        A->sort_key = g_strdup_printf("%04x%04x",
            A->vendor, A->device );
    if (!B->sort_key)
        B->sort_key = g_strdup_printf("%04x%04x",
            B->vendor, B->device );
    return g_strcmp0(A->sort_key, B->sort_key);
}

gboolean util_usb_ids_lookup_list(GSList *items) {
    FILE *usb_dot_ids;
    long last_vendor_fpos = 0, line_fpos = 0, id = 0;
    char buffer[128] = "";
    int found_count = 0;

    GSList *tmp = NULL, *l = NULL;

    usb_dot_ids = fopen("usb.ids", "r");
    if (!usb_dot_ids) return -1;

    tmp = g_slist_copy(items);
    tmp = g_slist_sort(tmp, (GCompareFunc)cmp_util_usb_id);

    l = tmp;
    while(l) {
        util_usb_id *d = l->data;
        while ( 1+(line_fpos = ftell(usb_dot_ids)) &&
                fgets(buffer, 128, usb_dot_ids)   ) {
            if (buffer[0] == '#') continue; /* comment */
            if (buffer[0] == '\n') continue; /* empty line */
            gboolean not_found = FALSE;

            char *next_sp = strchr(buffer, ' ');
            int tabs = 0;
            while(buffer[tabs] == '\t')
                tabs++;
            id = strtol(buffer, &next_sp, 16);

            switch (tabs) {
                case 0:  /* vendor  vendor_name */
                    if (id == d->vendor) {
                        found_count++;
                        d->vendor_str = g_strdup(g_strstrip(next_sp));
                        printf("usb.ids match for %s v:%s\n", d->address, d->vendor_str);
                        last_vendor_fpos = line_fpos;
                        continue;
                    } else if (id > d->vendor) not_found = TRUE;
                    break;
                case 1:  /* device  device_name */
                    if (!d->vendor_str) break;
                    if (id == d->device) {
                        d->device_str = g_strdup(g_strstrip(next_sp));
                        continue;
                    } else if (id > d->device) not_found = TRUE;
                    break;
                case 2:  /* interface  interface_name */
                    //TODO: interfaces
                    break;
            }
            if (not_found)
                break;
        }
        /* rewind a bit for the next device (maybe same vendor) */
        fseek(usb_dot_ids, last_vendor_fpos, SEEK_SET);
        l = l->next;
    }

    //TODO: class

    fclose(usb_dot_ids);
    return found_count;
}

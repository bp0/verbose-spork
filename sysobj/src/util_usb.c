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

#include <stdlib.h> /* for strtol() */
#include <string.h> /* for strchr() */
#include "sysobj.h"
#include "util_sysobj.h" /* for util_get_did() */
#include "util_usb.h"

/* [N-][P.P.P...][:C.I] */
gboolean verify_usb_interface(gchar *str) {
    gchar *p = str;
    if (!isdigit(*p)) return FALSE;
    int state = 0;
    while (*p != 0) {
        if ( isdigit(*p) ) { p++; continue; }
        switch(state) {
            case 0: if (*p == '-') { state = 1; } break;
            case 1:
                if (*p == ':') { state = 2; break; }
                if (*p != '.') { return FALSE; }
            case 2:
                if (*p != '.') { return FALSE; }
        }
        p++;
    }
    return TRUE;
}

/* usbN */
gboolean verify_usb_bus(gchar *str) {
    if (util_get_did(str, "usb") >= 0)
        return TRUE;
    return FALSE;
}

/* [N-][P.P.P...] */
gboolean verify_usb_device(gchar *str) {
    gchar *p = str;
    if (!isdigit(*p)) return FALSE;
    int state = 0;
    while (*p != 0) {
        if ( isdigit(*p) ) { p++; continue; }
        switch(state) {
            case 0: if (*p == '-') { state = 1; } break;
            case 1: if (*p != '.') { return FALSE; }
        }
        p++;
    }
    return TRUE;
}

void util_usb_id_free(util_usb_id *s) {
    if (s) {
        g_free(s->address);
        g_free(s->sort_key);
        g_free(s->vendor_str);
        g_free(s->device_str);
        g_free(s);
    }
}

int util_usb_ids_lookup(util_usb_id *usbd) {
    GSList *tmp = NULL;
    tmp = g_slist_append(tmp, usbd);
    int ret = util_usb_ids_lookup_list(tmp);
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

    gchar *usbids_file = sysobj_find_data_file("usb.ids");
    if (!usbids_file) {
        //_msg("usb.ids file not found");
        return 0;
    }

    usb_dot_ids = fopen(usbids_file, "r");
    g_free(usbids_file);
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

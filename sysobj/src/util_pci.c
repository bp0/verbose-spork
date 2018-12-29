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
#include <ctype.h>  /* for isxdigit() */
#include "sysobj.h"
#include "util_pci.h"

/* ????:??:??.? */
gboolean verify_pci_addy(gchar *str) {
    if (!str) return FALSE;
    /* may not be needed, won't isxdigit(0) fail
     * and short-circuit the check? */
    if (strlen(str) != 12) return FALSE;
    if (  isxdigit(str[0])
        && isxdigit(str[1])
        && isxdigit(str[2])
        && isxdigit(str[3])
        && str[4] == ':'
        && isxdigit(str[5])
        && isxdigit(str[6])
        && str[7] == ':'
        && isxdigit(str[8])
        && isxdigit(str[9])
        && str[10] == '.'
        && isxdigit(str[11])
        && str[12] == 0 )
            return TRUE;
    return FALSE;
}

void util_pci_id_free(util_pci_id *s) {
    if (s) {
        g_free(s->address);
        g_free(s->sort_key);
        g_free(s->vendor_str);
        g_free(s->device_str);
        g_free(s->sub_vendor_str);
        g_free(s->sub_device_str);
        g_free(s->dev_class_str);
        g_free(s->dev_subclass_str);
        g_free(s->dev_progif_str);
        g_free(s);
    }
}

int util_pci_ids_lookup(util_pci_id *pcid) {
    GSList *tmp = NULL;
    tmp = g_slist_append(tmp, pcid);
    int ret = util_pci_ids_lookup_list(tmp);
    g_slist_free(tmp);
    return ret;
}

static int cmp_util_pci_id_sub(const gpointer a, const gpointer b) {
    util_pci_id *A = a ? a : NULL;
    util_pci_id *B = b ? b : NULL;
    if (!A) return (B ? -1 : 0);
    if (!B) return (A ? 1 : 0);
    return (A->sub_vendor > B->sub_vendor) - (A->sub_vendor < B->sub_vendor);
}

static int cmp_util_pci_id_class(const gpointer a, const gpointer b) {
    util_pci_id *A = a ? a : NULL;
    util_pci_id *B = b ? b : NULL;
    if (!A) return (B ? -1 : 0);
    if (!B) return (A ? 1 : 0);
    return (A->dev_class > B->dev_class) - (A->dev_class < B->dev_class);
}

static int cmp_util_pci_id(const gpointer a, const gpointer b) {
    util_pci_id *A = a ? a : NULL;
    util_pci_id *B = b ? b : NULL;
    if (!A) return (B ? -1 : 0);
    if (!B) return (A ? 1 : 0);
    if (!A->sort_key)
        A->sort_key = g_strdup_printf("%04x%04x%04x%04x",
            A->vendor, A->device, A->sub_vendor, A->sub_device );
    if (!B->sort_key)
        B->sort_key = g_strdup_printf("%04x%04x%04x%04x",
            B->vendor, B->device, B->sub_vendor, B->sub_device );
    return g_strcmp0(A->sort_key, B->sort_key);
}

int util_pci_ids_lookup_list(GSList *items) {
    FILE *pci_dot_ids;
    long last_vendor_fpos = 0, line_fpos = 0, id = 0, id2 = 0;
    char buffer[128] = "";
    int found_count = 0;

    GSList *tmp = NULL, *l = NULL;

    gchar *pcids_file = sysobj_find_data_file("pci.ids");
    if (!pcids_file) {
        //_msg("pci.ids file not found");
        return 0;
    }

    pci_dot_ids = fopen(pcids_file, "r");
    g_free(pcids_file);
    if (!pci_dot_ids) return -1;

    tmp = g_slist_copy(items);
    tmp = g_slist_sort(tmp, (GCompareFunc)cmp_util_pci_id);

    l = tmp;
    while(l) {
        util_pci_id *d = l->data;
        while ( 1+(line_fpos = ftell(pci_dot_ids)) &&
                fgets(buffer, 128, pci_dot_ids)   ) {
            if (buffer[0] == '#') continue; /* comment */
            if (buffer[0] == '\n') continue; /* empty line */
            if (buffer[0] == 'C'
                && buffer[1] == ' ') break; /* arrived at class section */
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
                case 2:  /* subvendor subdevice  subsystem_name */
                    if (! (d->vendor_str && d->device_str) ) break;
                    if (id == d->sub_vendor) {
                        id2 = strtol(next_sp, NULL, 16);
                        next_sp = strchr(g_strstrip(next_sp), ' ');
                        if (id2 == d->sub_device && next_sp) {
                            d->sub_device_str = g_strdup(g_strstrip(next_sp));
                            continue;
                        } else if (id2 > d->sub_device)
                            not_found = TRUE;
                    } else if (id > d->sub_vendor) not_found = TRUE;
                    break;
            }
            if (not_found)
                break;
        }
        /* rewind a bit for the next device (maybe same vendor) */
        fseek(pci_dot_ids, last_vendor_fpos, SEEK_SET);
        l = l->next;
    }

    /* second pass for the subvendor ids */
    tmp = g_slist_sort(tmp, (GCompareFunc)cmp_util_pci_id_sub);
    last_vendor_fpos = 0;
    fseek(pci_dot_ids, 0, SEEK_SET);
    l = tmp;
    while(l) {
        util_pci_id *d = l->data;
        while ( 1+(line_fpos = ftell(pci_dot_ids)) &&
                fgets(buffer, 128, pci_dot_ids)   ) {
            if (buffer[0] == '#') continue; /* comment */
            if (buffer[0] == '\n') continue; /* empty line */
            if (buffer[0] == 'C'
                && buffer[1] == ' ') break; /* arrived at class section */
            gboolean not_found = FALSE;

            char *next_sp = strchr(buffer, ' ');
            int tabs = 0;
            while(buffer[tabs] == '\t')
                tabs++;
            id = strtol(buffer, &next_sp, 16);

            switch (tabs) {
                case 0:  /* vendor  vendor_name */
                    if (id == d->sub_vendor) {
                        d->sub_vendor_str = g_strdup(g_strstrip(next_sp));
                        last_vendor_fpos = line_fpos;
                        continue;
                    } else if (id > d->sub_vendor) not_found = TRUE;
                    break;
            }
            if (not_found)
                break;
        }
        /* rewind a bit for the next device (maybe same vendor) */
        fseek(pci_dot_ids, last_vendor_fpos, SEEK_SET);

        l = l->next;
    }

    /* third pass for the classes */
    tmp = g_slist_sort(tmp, (GCompareFunc)cmp_util_pci_id_class);
    last_vendor_fpos = 0; /* reuse vendor as class */
    fseek(pci_dot_ids, 0, SEEK_SET);
    gboolean found_classes = FALSE;
    l = tmp;
    while(l) {
        util_pci_id *d = l->data;
        int this_class = (d->dev_class >> 16) & 0xff;
        int this_subclass = (d->dev_class >> 8) & 0xff;
        int this_progif = d->dev_class & 0xff;
        while ( 1+(line_fpos = ftell(pci_dot_ids)) &&
                fgets(buffer, 128, pci_dot_ids)   ) {
            if (buffer[0] == '#') continue; /* comment */
            if (buffer[0] == '\n') continue; /* empty line */
            if (buffer[0] == 'C'
                && buffer[1] == ' ') found_classes = TRUE; /* arrived at class section */
            if (!found_classes) continue;
            gboolean not_found = FALSE;

            char *next_sp = strchr(buffer, ' ');
            int tabs = 0;
            while(buffer[tabs] == '\t')
                tabs++;
            if (tabs == 0 && buffer[0] == 'C' && buffer[1] == ' ')
                id = strtol(buffer + 2, &next_sp, 16);
            else
                id = strtol(buffer, &next_sp, 16);

            switch (tabs) {
                case 0:  /* class  class_name */
                    if (id == this_class ) {
                        d->dev_class_str = g_strdup(g_strstrip(next_sp));
                        last_vendor_fpos = line_fpos;
                        continue;
                    } else if (id > this_class) not_found = TRUE;
                    break;
                case 1:  /* subclass  sub_class_name */
                    if (!d->dev_class_str) break;
                    if (id == this_subclass ) {
                        d->dev_subclass_str = g_strdup(g_strstrip(next_sp));
                        continue;
                    } else if (id > this_subclass) not_found = TRUE;
                    break;
                case 2:  /* prog-if  prog-if_name */
                    if (!d->dev_subclass_str) break;
                    if (id == this_progif ) {
                        d->dev_progif_str = g_strdup(g_strstrip(next_sp));
                        continue;
                    } else if (id > this_progif) not_found = TRUE;
                    break;
            }
            if (not_found)
                break;
        }
        /* rewind a bit for the next device (maybe same class) */
        fseek(pci_dot_ids, last_vendor_fpos, SEEK_SET);

        l = l->next;
    }

    /* one more pass to fill in missing info */
    for(l = items; l; l = l->next) {
        util_pci_id *d = l->data;
        /* use class as device name, if no device name was found */
        if (!d->device_str) {
            if (d->dev_subclass_str)
                d->device_str = g_strdup(d->dev_subclass_str);
            else if (d->dev_class_str)
                d->device_str = g_strdup(d->dev_class_str);
        }
        if (!d->sub_device_str) {
            if (d->dev_subclass_str)
                d->sub_device_str = g_strdup(d->dev_subclass_str);
            else if (d->dev_class_str)
                d->sub_device_str = g_strdup(d->dev_class_str);
        }
    }

    fclose(pci_dot_ids);
    return found_count;
}

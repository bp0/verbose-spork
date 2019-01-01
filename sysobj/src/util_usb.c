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

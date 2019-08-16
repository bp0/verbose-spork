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

#include "gettext.h"
#include "util_edid.h"
#include "util_sysobj.h"
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

int edid_fill(struct edid *id_out, const void *edid_bytes, int edid_len) {
    if (!id_out) return 0;

    memset(id_out, 0, sizeof(struct edid));

    if (edid_len >= 128) {
        uint8_t *u8 = (uint8_t*)edid_bytes;
        uint16_t *u16 = (uint16_t*)edid_bytes;
        uint32_t *u32 = (uint32_t*)edid_bytes;

        uint16_t vid = be16toh(u16[4]); /* bytes 8-9 */
        id_out->ven[2] = 64 + (vid & 0x1f);
        id_out->ven[1] = 64 + ((vid >> 5) & 0x1f);
        id_out->ven[0] = 64 + ((vid >> 10) & 0x1f);

        id_out->product = le16toh(u16[5]); /* bytes 10-11 */
        id_out->n_serial = le32toh(u32[3]);/* bytes 12-15 */
        id_out->week = u8[16];             /* byte 16 */
        id_out->year = u8[17] + 1990;      /* byte 17 */
        id_out->ver_major = u8[18];        /* byte 18 */
        id_out->ver_minor = u8[19];        /* byte 19 */

        id_out->a_or_d = (u8[20] & 0x80) ? 1 : 0;
        if (id_out->a_or_d == 1) {
            /* digital */
            switch((u8[20] >> 4) & 0x7) {
                case 0x1: id_out->bpc = 6; break;
                case 0x2: id_out->bpc = 8; break;
                case 0x3: id_out->bpc = 10; break;
                case 0x4: id_out->bpc = 12; break;
                case 0x5: id_out->bpc = 14; break;
                case 0x6: id_out->bpc = 16; break;
            }
        }

        if (u8[21] && u8[22]) {
            id_out->horiz_cm = u8[21];
            id_out->vert_cm = u8[22];
            id_out->diag_cm =
                sqrt( (id_out->horiz_cm * id_out->horiz_cm)
                 + (id_out->vert_cm * id_out->vert_cm) );
            id_out->diag_in = id_out->diag_cm / 2.54;
        }

        id_out->check = u8[127];

        uint16_t dh, dl;

        /* descriptor at byte 54 */
        dh = be16toh(u16[54/2]);
        dl = be16toh(u16[54/2+1]);
        id_out->d_type[0] = (dh << 16) + dl;
        switch(id_out->d_type[0]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[0], u8+54+5, 13);
        }

        /* descriptor at byte 72 */
        dh = be16toh(u16[72/2]);
        dl = be16toh(u16[72/2+1]);
        id_out->d_type[1] = (dh << 16) + dl;
        switch(id_out->d_type[1]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[1], u8+72+5, 13);
        }

        /* descriptor at byte 90 */
        dh = be16toh(u16[90/2]);
        dl = be16toh(u16[90/2+1]);
        id_out->d_type[2] = (dh << 16) + dl;
        switch(id_out->d_type[2]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[2], u8+90+5, 13);
        }

        /* descriptor at byte 108 */
        dh = be16toh(u16[108/2]);
        dl = be16toh(u16[108/2+1]);
        id_out->d_type[3] = (dh << 16) + dl;
        switch(id_out->d_type[3]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[3], u8+108+5, 13);
        }

        for(int i = 0; i < 4; i++) {
            g_strstrip(id_out->d_text[i]);
            switch(id_out->d_type[i]) {
                case 0xfc:
                    id_out->name = id_out->d_text[i];
                    break;
                case 0xff:
                    id_out->serial = id_out->d_text[i];
                    break;
                case 0xfe:
                    if (id_out->ut1)
                        id_out->ut2 = id_out->d_text[i];
                    else
                        id_out->ut1 = id_out->d_text[i];
                    break;
            }
        }

        if (!id_out->name) {
            /* LG may use "uspecified text" for name and model */
            //SEQ(id_out->ven, "LGD")
            if (SEQ(id_out->ut1, "LG Display") && id_out->ut2)
                id_out->name = id_out->ut2;
            else {
                if (id_out->ut1) id_out->name = id_out->ut1;
                if (id_out->ut2 && !id_out->serial) id_out->serial = id_out->ut2;
            }
        }
    }
    return 1;
}

int edid_fill_xrandr(struct edid *id_out, const char *xrandr_edid_dump) {
    uint8_t buffer[256], *n = buffer;
    int len = 0;

    const char *p = xrandr_edid_dump;
    char byte[3] = "..";

    while(p && *p) {
        if (isxdigit(p[0]) && isxdigit(p[1])) {
            byte[0] = p[0];
            byte[1] = p[1];
            *n = strtol(byte, NULL, 16);
            n++;
            len++;
            p += 2;
        } else
            p++;
    }

    return edid_fill(id_out, buffer, len);
}

const char *edid_descriptor_type(int type) {
    switch(type) {
        case 0xff:
            return N_("display serial number");
        case 0xfe:
            return N_("unspecified text");
        case 0xfd:
            return N_("display range limits");
        case 0xfc:
            return N_("display name");
        case 0xfb:
            return N_("additional white point");
        case 0xfa:
            return N_("additional standard timing identifiers");
        case 0xf9:
            return N_("Display Color Management");
        case 0xf8:
            return N_("CVT 3-byte timing codes");
        case 0xf7:
            return N_("additional standard timing");
        case 0x10:
            return N_("dummy");
    }
    if (type < 0x0f) return N_("manufacturer reserved descriptor");
    return N_("detailed timing descriptor");
}

char *edid_dump(struct edid *id) {
    char *ret = NULL;
    if (!id) return NULL;
    ret = appf(ret, "\n", "edid_version: %d.%d", id->ver_major, id->ver_minor);
    ret = appf(ret, "\n", "mfg: %s, model: %u, n_serial: %u, dom: week %d of %d", id->ven, id->product, id->n_serial, id->week, id->year);

    ret = appf(ret, "\n", "type: %s", id->a_or_d ? "digital" : "analog");
    if (id->bpc)
        ret = appf(ret, "\n", "bits per color channel: %d", id->bpc);

    if (id->horiz_cm && id->vert_cm)
        ret = appf(ret, "\n", "size: %d cm × %d cm", id->horiz_cm, id->vert_cm);
    if (id->diag_cm)
        ret = appf(ret, "\n", "diagonal: %0.2f cm (%0.2f in)", id->diag_cm, id->diag_in);
    ret = appf(ret, "\n", "checkbyte: %d", id->check);

    for(int i = 0; i < 4; i++)
        ret = appf(ret, "\n", "descriptor[%d] (%s): %s", i, _(edid_descriptor_type(id->d_type[i])), *id->d_text[i] ? id->d_text[i] : "{...}");
    return ret;
}

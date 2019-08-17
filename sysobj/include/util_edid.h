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

#ifndef __UTIL_EDID_H__
#define __UTIL_EDID_H__

#include <stdint.h>  /* for *int*_t types */

#define EDID_MAX_EXT_BLOCKS 254

struct edid_dtd {
    uint8_t *ptr;
};

struct edid_cea_block {
    uint8_t *ptr;
    int type, len;
};

typedef struct {
    union {
        void* data;
        uint8_t* u8;
        uint16_t* u16;
        uint32_t* u32;
    };
    unsigned int len;
    int ver_major, ver_minor;
    int checksum_ok; /* first 128-byte block only */
    int ext_blocks, ext_blocks_ok, ext_blocks_fail;

    int dtd_count;
    struct edid_dtd *dtds;

    int cea_block_count;
    struct edid_cea_block *cea_blocks;

    int d_type[4];
    char d_text[4][14];
    /* point into d_text */
    char *name;
    char *serial;
    char *ut1;
    char *ut2;

} edid;
edid *edid_new(const char *data, unsigned int len);
edid *edid_new_from_hex(const char *hex_string);
edid *edid_free(edid *e);
char *edid_dump_hex(edid *e, int tabs, int breaks);

/* just enough edid decoding */
typedef struct {
    int ver_major, ver_minor;
    char ven[4];

    int d_type[4];
    char d_text[4][14];

    /* point into d_text */
    char *name;
    char *serial;
    char *ut1;
    char *ut2;

    int a_or_d; /* 0 = analog, 1 = digital */
    int bpc;

    uint16_t product;
    uint32_t n_serial;
    int week, year;

    int horiz_cm, vert_cm;
    float diag_cm, diag_in;

    int size; /* bytes */
    int checksum_ok; /* block 0 */

    int ext_blocks, ext_blocks_ok, ext_blocks_fail;
    uint8_t ext[EDID_MAX_EXT_BLOCKS][2]; /* block type, checksum_ok */
} edid_basic;

int edid_fill(edid_basic *id_out, const void *edid_bytes, int edid_len);
int edid_fill2(edid_basic *id_out, edid* e);

const char *edid_descriptor_type(int type);
char *edid_dump(edid_basic *id);
char *edid_dump2(edid *e);

#endif

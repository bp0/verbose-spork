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

#define _GNU_SOURCE
#include <stdint.h>  /* for *int*_t types */
#include <glib.h>

typedef struct _V2_EDID V2_EDID;

enum {
    EDID_BLK_INVALID    = 0,
    EDID_BLK_BASE       = 0x01,
    EDID_BLK_HEADER     = 0x02,
    EDID_BLK_PROD       = 0x03,
    EDID_BLK_VERSION    = 0x04,
    EDID_BLK_PROD_CODE  = 0x05,
    EDID_BLK_PROD_SERIAL= 0x06,
    EDID_BLK_DESC       = 0x07, /* display descriptor that is not a DTD */

    EDID_BLK_VENDOR_PNP3 = 0x51, /* 3 chars, 2 bytes */
    EDID_BLK_VENDOR_PNP4 = 0x52, /* 4 chars, 3 bytes */
    EDID_BLK_VENDOR_OUI  = 0x53,

    EDID_BLK_DOM90      = 0x31, /* EDID, base year 1990 */
    EDID_BLK_DOM2K      = 0x32, /* DisplayID, base year 2000 */

    EDID_BLK_EXT        = 0x10,
    EDID_BLK_EXT_MAP    = 0x11,
    EDID_BLK_EXT_CEA    = 0x12,
    EDID_BLK_EXT_DID    = 0x13,
    EDID_BLK_EXT_DI     = 0x14,

    EDID_BLK_ETB        = 0x80,
    EDID_BLK_STD        = 0x81,
    EDID_BLK_DTD        = 0x82,
    EDID_BLK_SVD        = 0x83,
    EDID_BLK_DID_T1     = 0x85,
    EDID_BLK_DID_T2     = 0x86,
    EDID_BLK_DID_T3     = 0x87,
    EDID_BLK_DID_T4     = 0x88,
    EDID_BLK_DID_T5     = 0x89,
    EDID_BLK_DID_T6     = 0x8a,
    EDID_BLK_DID_T7     = 0x8b,
    EDID_BLK_DID_T8_1   = 0x8c,
    EDID_BLK_DID_T8_2   = 0x8d,
    EDID_BLK_DID_T9     = 0x8e,

    EDID_BLK_SAD        = 0x41,
    EDID_BLK_SPEAKERS   = 0x42,

    EDID_BLK_STR        = 0x20,
    EDID_BLK_STR_NAME   = 0x21,
    EDID_BLK_STR_SERIAL = 0x22,

    EDID_BLK_CEA        = 0x60,

    EDID_BLK_DID        = 0x70,

    EDID_BLK_VSPEC      = 0xe0,
    EDID_BLK_PHYS_ADDY  = 0xe1,  /* HDMI CEC Physical Address */
};

typedef struct {
    V2_EDID *e;
    uint8_t type; /* enum EDID_BLK_* */
    uint16_t first;
    uint16_t length;
    uint16_t header_length; /* non-payload bytes at start of block */
    uint16_t footer_length; /* non-payload bytes at end of block */
    uint16_t parent; /* parent's index+1, 0 is none, n is blocks[n-1] */
    int8_t checksum_ok; /* 0: no, 1: yes, -1: n/a */
    uint8_t bounds_ok;
    uint8_t error; /* something is wrong inside this block
                    * TODO: perhaps type << 8 + error could be decoded. */

    uint16_t bi; /* self */
    uint16_t ti; /* if the block is a single timing */
    void *decoded;
} V2_EDIDBlock;

typedef struct {
    V2_EDID *e;
    uint16_t bi;
    char *name;
    char *value;
    uint32_t offset, size, mask;
} V2_EDIDBlockAttribute;

enum {
    VEN_TYPE_INVALID = 0,
    VEN_TYPE_PNP,
    VEN_TYPE_OUI,
};

/* OUI is stored in EDID as 24-bit little-endian,
 * but lookup file from IEEE expects big-endian.
 * .oui_str is .oui rendered into big-endian for
 * easy lookup. */
typedef struct {
    union {
        char pnp[7]; /* only needs 3+null */
        char oui_str[7]; /* needs 6+null */
    };
    uint32_t oui;
    uint8_t type; /* enum VEN_TYPE_* */
} V2_EDIDVendor;

/* order by rising priority */
enum {
    OUTSRC_SVD_UNK = -3,
    OUTSRC_MSP     = -2,
    OUTSRC_INVALID = -1,
    OUTSRC_EDID    =  0,
    OUTSRC_ETB,
    OUTSRC_STD,
    OUTSRC_AST3B,
    OUTSRC_DTD,
    OUTSRC_CEA_DTD,
    OUTSRC_SVD,

    OUTSRC_DID_TYPE_I,
    OUTSRC_DID_TYPE_VI,
    OUTSRC_DID_TYPE_VII,
};

typedef struct {
    V2_EDID *e;
    uint16_t bi, ai, ti;

    int8_t src;

    uint32_t horiz_pixels, vert_pixels, vert_lines;
    double rate_hz;
    uint8_t is_interlaced;

    double pixel_clock_khz;

    uint32_t horiz_active, horiz_blanking, horiz_fporch, horiz_total;
    uint32_t vert_active, vert_blanking, vert_border, vert_total;

} V2_EDIDTiming;

enum {
    STD_EDID         = 0,
    STD_EEDID        = 1,
    STD_EIACEA861    = 2,
    STD_DISPLAYID    = 3,
    STD_DISPLAYID20  = 4,
};

struct _V2_EDID {
    union {
        void* data;
        uint8_t* u8;
    };
    unsigned int len;

    uint8_t ver_major, ver_minor;
    uint8_t extends_to; /* enum STD_* */

    uint16_t block_count;
    V2_EDIDBlock *blocks;

    uint16_t attr_count;
    V2_EDIDBlockAttribute *attrs;

    uint16_t timing_count;
    V2_EDIDTiming *timings;

    GString *msg_log;
};

/*******************************************************************/

typedef struct _edid edid;

typedef struct {
    edid *e;
    uint32_t offset;
} edid_addy;

/* OUI is stored in EDID as 24-bit little-endian,
 * but lookup file from IEEE expects big-endian.
 * .oui_str is .oui rendered into big-endian for
 * easy lookup. */
typedef struct {
    union {
        char pnp[7]; /* only needs 3+null */
        char oui_str[7]; /* needs 6+null */
    };
    uint32_t oui;
    uint8_t type; /* enum VEN_TYPE_* */
} edid_ven;

typedef struct {
    char *str;
    uint16_t len;
    uint8_t is_product_name;
    uint8_t is_serial;
} DisplayIDString;

typedef struct {
    uint8_t version;
    uint8_t extension_length;
    uint8_t primary_use_case;
    uint8_t extension_count;
    uint16_t blocks;
    uint8_t checksum_ok;
} DisplayIDMeta;

typedef struct {
    edid_addy addy;
    union {
        uint8_t tag;
        uint8_t type;
    };
    uint8_t revision;
    uint8_t len;
    uint8_t bounds_ok;

    /* for vendor specific block */
    edid_ven ven;
} DisplayIDBlock;

typedef struct {
    edid_addy addy;
    uint8_t interface;
    uint8_t supports_hdcp;
    uint8_t exists;
} DIExtData;

typedef struct {
    float horiz_cm, vert_cm;
    float diag_cm, diag_in;
    int horiz_blanking, vert_blanking;
    int horiz_pixels, vert_lines, vert_pixels;
    float vert_freq_hz;
    uint8_t is_interlaced;
    uint8_t is_preferred;
    int stereo_mode;
    uint64_t pixel_clock_khz;
    int src; /* enum OUTSRC_* */
    uint64_t pixels; /* h*v: easier to compare */
    char class_inch[12];
} edid_output;

struct edid_std {
    uint8_t *ptr;
    edid_output out;
};

struct edid_dtd {
    edid_addy addy;
    uint8_t cea_ext; /* in a CEA block vs one of the regular EDID descriptors */
    edid_output out;
    uint8_t bounds_ok;
};

struct edid_svd {
    uint8_t v;
    uint8_t is_native;
    edid_output out;
};

struct edid_sad {
    uint8_t v[3];
    uint8_t format, channels, freq_bits;
    int depth_bits; /* format 1 */
    int max_kbps;   /* formats 2-8 */
};

struct edid_cea_block {
    edid_addy addy;
    int type, len;
    uint8_t bounds_ok;

    /* for vendor specific block */
    edid_ven ven;
};

struct edid_descriptor {
    edid_addy addy;
    uint8_t type;
    char text[14];
};

struct edid_manf_date {
    uint8_t week;
    uint8_t is_model_year; /* ignore week */
    uint16_t year;
    int std; /* enum STD_* */
};

typedef struct _edid {
    union {
        void* data;
        uint8_t* u8;
        uint16_t* u16;
    };
    unsigned int len;

    int std; /* enum STD_* */

    uint8_t ver_major, ver_minor;
    uint8_t checksum_ok; /* first 128-byte block only */
    uint8_t ext_blocks, ext_blocks_ok, ext_blocks_fail;
    uint8_t *ext_ok;

    int etb_count;
    edid_output etbs[24];

    int std_count;
    struct edid_std stds[8];

    int dtd_count;
    struct edid_dtd *dtds;

    int cea_block_count;
    struct edid_cea_block *cea_blocks;

    int svd_count;
    struct edid_svd *svds;

    int sad_count;
    struct edid_sad *sads;

    edid_ven ven;
    struct edid_descriptor d[4];
    /* point into d[].text */
    char *name;
    char *serial;
    char *ut1;
    char *ut2;

    uint8_t a_or_d; /* 0 = analog, 1 = digital */
    uint8_t interface; /* digital interface */
    uint8_t bpc;       /* digital bpc */
    uint16_t product;
    uint32_t n_serial;
    struct edid_manf_date dom;
    edid_output img;
    edid_output img_svd;
    edid_output img_max;
    uint32_t speaker_alloc_bits;

    DIExtData di;

    DisplayIDMeta did;
    int did_block_count;
    DisplayIDBlock *did_blocks;
    int did_string_count;
    DisplayIDString *did_strings;

    int didt_count;
    edid_output *didts;

    V2_EDID *e2;

    GString *msg_log;
} edid;
edid *edid_new(const char *data, unsigned int len);
edid *edid_new_from_hex(const char *hex_string);
edid *edid_new_from_file(const char *path);
void edid_free(edid *e);
char *edid_dump_hex(edid *e, int tabs, int breaks);

V2_EDID *V2_edid_new(const char *data, unsigned int len);
V2_EDID *V2_edid_new_from_hex(const char *hex_string);
V2_EDID *V2_edid_new_from_file(const char *path);
void V2_edid_free(V2_EDID *e);
char *V2_edid_dump_hex(V2_EDID *e, int tabs, int breaks);

const char *edid_standard(int std);
const char *edid_output_src(int src);
const char *edid_interface(int type);
const char *edid_di_interface(int type);
const char *edid_descriptor_type(int type);
const char *edid_ext_block_type(int type);
const char *edid_cea_block_type(int type);
const char *edid_cea_audio_type(int type);
const char *edid_did_block_type(int type);

char *edid_output_describe(edid_output *out);
char *edid_base_descriptor_describe(struct edid_descriptor *d);
char *edid_dtd_describe(struct edid_dtd *dtd, int dump_bytes);
char *edid_cea_block_describe(struct edid_cea_block *blk);
char *edid_cea_audio_describe(struct edid_sad *sad);
char *edid_cea_speaker_allocation_describe(int bitfield, int short_version);
char *edid_did_block_describe(DisplayIDBlock *blk);

char *edid_dump2(edid *e);
char *V2_edid_dump(V2_EDID *e);

#endif

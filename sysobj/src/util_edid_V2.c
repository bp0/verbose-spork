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

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <endian.h>
#include <stdio.h>
#include "gettext.h"
#include "util_edid.h"
#include "util_sysobj.h"

#define NOMASK (~0U)
#define BFMASK(LSB, MASK) (MASK << LSB)

#define DPTR(ADDY) (uint8_t*)(&((ADDY).e->u8[(ADDY).offset]))
#define OFMT "@%03d" /* for addy.offset */

#define EDID_MSG_STDERR 0
#define edid_msg(e, msg, ...) {\
    if (EDID_MSG_STDERR) fprintf (stderr, ">[%s;L%d] " msg "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    g_string_append_printf(e->msg_log, "[%s;L%d] " msg "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); }

#define ADD_BLOCK(EPTR, ...) { EPTR->blocks[EPTR->block_count++] = (V2_EDIDBlock){.e = EPTR, __VA_ARGS__}; }

static int str_make_printable(char *str) {
    int rc = 0;
    char *p;
    for(p = str; *p; p++) {
        if (!isprint(*p)) {
            *p = '.';
            rc++;
        }
    }
    return rc;
}

static inline
uint32_t bf_value(uint32_t value, uint32_t mask) {
    uint32_t result = value & mask;
    if (result)
        while(!(mask & 1)) {
            result >>= 1;
            mask >>= 1;
        }
    return result;
}

static inline
uint8_t bounds_check(V2_EDID *e, uint32_t offset) {
    if (!e) return 0;
    if (offset > e->len) return 0;
    return 1;
}

static inline
char *rstr(V2_EDID *e, uint32_t offset, uint32_t len) {
    if (!bounds_check(e, offset+len)) return NULL;
    char *raw = malloc(len+1), *ret = NULL;
    strncpy(raw, (char*)&e->u8[offset], len);
    raw[len] = 0;
    ret = g_utf8_make_valid(raw, len);
    g_free(raw);
    return ret;
}

static inline
char *rstr_strip(V2_EDID *e, uint32_t offset, uint32_t len) {
    if (!bounds_check(e, offset+len)) return NULL;
    char *raw = malloc(len+1), *ret = NULL;
    strncpy(raw, (char*)&e->u8[offset], len);
    raw[len] = 0;
    ret = g_strstrip(g_utf8_make_valid(raw, len));
    g_free(raw);
    return ret;
}

static inline
uint32_t r8(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset)) return 0;
    return bf_value(e->u8[offset], mask);
}

static inline
uint32_t r16le(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+1)) return 0;
    uint32_t v = (e->u8[offset+1] << 8) + e->u8[offset];
    return bf_value(v, mask);
}

static inline
uint32_t r16be(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+1)) return 0;
    uint32_t v = (e->u8[offset] << 8) + e->u8[offset+1];
    return bf_value(v, mask);
}

static inline
uint32_t r24le(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+2)) return 0;
    uint32_t v = (e->u8[offset+2] << 16) + (e->u8[offset+1] << 8) + e->u8[offset];
    return bf_value(v, mask);
}

static inline
uint32_t r24be(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+2)) return 0;
    uint32_t v = (e->u8[offset] << 16) + (e->u8[offset+1] << 8) + e->u8[offset+2];
    return bf_value(v, mask);
}

static inline
uint32_t r32le(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+3)) return 0;
    uint32_t v = (e->u8[offset+3] << 24) + (e->u8[offset+2] << 16)
        + (e->u8[offset+1] << 8) + e->u8[offset];
    return bf_value(v, mask);
}

static inline
uint32_t r32be(V2_EDID *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+3)) return 0;
    uint32_t v = (e->u8[offset] << 24) + (e->u8[offset+1] << 16)
        + (e->u8[offset+2] << 8) + e->u8[offset+3];
    return bf_value(v, mask);
}

static inline
int rpnpcpy(V2_EDIDVendor *dest, V2_EDID *e, uint32_t offset) {
    uint32_t pnp = r16be(e, offset, NOMASK);
    V2_EDIDVendor ret = {.type = VEN_TYPE_INVALID};
    if (pnp) {
        ret.type = VEN_TYPE_PNP;
        ret.pnp[2] = 64 + (pnp & 0x1f);
        ret.pnp[1] = 64 + ((pnp >> 5) & 0x1f);
        ret.pnp[0] = 64 + ((pnp >> 10) & 0x1f);
        *dest = ret;
        return 1;
    }
    return 0;
}

static inline
int rouicpy(V2_EDIDVendor *dest, V2_EDID *e, uint32_t offset) {
    V2_EDIDVendor ret = {.type = VEN_TYPE_OUI};
    ret.oui = r24le(e, offset, NOMASK);
    sprintf(ret.oui_str, "%02x%02x%02x",
        (ret.oui >> 16) & 0xff,
        (ret.oui >> 8) & 0xff,
            ret.oui & 0xff );
    if (ret.oui) {
        *dest = ret;
        return 1;
    }
    return 0;
}

static int _block_check_n(const void *bytes, int len) {
    if (!bytes) return 0;
    uint8_t sum = 0;
    uint8_t *data = (uint8_t*)bytes;
    int i;
    for(i=0; i<len; i++)
        sum += data[i];
    return sum == 0 ? 1 : 0;
}

static int block_check_n(V2_EDID *e, uint32_t offset, int len) {
    if (!bounds_check(e, offset+len)) return 0;
    return _block_check_n(e->u8, len);
}

static int block_check(V2_EDID *e, uint32_t offset) {
    if (!bounds_check(e, offset+128)) return 0;
    return _block_check_n(e->u8, 128);
}

static char *hex_bytes(uint8_t *bytes, int count) {
    char *buffer = malloc(count*3+1), *p = buffer;
    memset(buffer, 0, count*3+1);
    int i;
    for(i = 0; i < count; i++) {
        sprintf(p, "%02x ", (unsigned int)bytes[i]);
        p += 3;
    }
    return buffer;
}

static void walk_cea_block(V2_EDID *e, uint16_t bi) {
    V2_EDIDBlock *blk = &e->blocks[bi];
    uint8_t *u8 = e->u8;
    int f = blk->first;
    uint16_t pi = bi+1;


}

static void walk_did_block(V2_EDID *e, uint16_t bi) {
    V2_EDIDBlock *blk = &e->blocks[bi];
    uint8_t *u8 = e->u8;
    int f = blk->first;
    uint16_t pi = bi+1;

    int b;

    switch(u8[f]) {
        case 0x03: /* Type I Detailed Timing */
            ADD_BLOCK(e,
                .type = EDID_BLK_DID_T1, .first = f+3, .length = blk->length-3,
                .parent = pi, .checksum_ok = -1, .bounds_ok = blk->bounds_ok);
            break;
        case 0x81: /* CTA DisplayID, ... Embedded CEA Blocks */
            b = 3;
            while(b < blk->length) {
                int db_type = (u8[f+b] & 0xe0) >> 5;
                int db_size =  u8[f+b] & 0x1f;
                int nbi = e->block_count;
                ADD_BLOCK(e,
                    .type = EDID_BLK_CEA, .first = f+b, .length = 1+db_size,
                    .parent = pi, .checksum_ok = -1, .bounds_ok = blk->bounds_ok,
                    .header_length = 1);
                walk_cea_block(e, nbi);
                b += db_size + 1;
            }
            break;
    }
}

static void walk_ext_block(V2_EDID *e, uint16_t bi) {
    V2_EDIDBlock *blk = &e->blocks[bi];
    uint8_t *u8 = e->u8;
    int f = blk->first;
    uint16_t pi = bi+1;

    int b, db_end;

    switch(blk->type) {
        case EDID_BLK_EXT_CEA:
            if (!blk->bounds_ok) return;
            db_end = u8[f+2];
            if (db_end) {
                b = 4;
                while(b < db_end) {
                    int db_type = (u8[f+b] & 0xe0) >> 5;
                    int db_size =  u8[f+b] & 0x1f;
                    int nbi = e->block_count;
                    ADD_BLOCK(e,
                        .type = EDID_BLK_CEA, .first = f+b, .length = 1+db_size,
                        .parent = pi, .checksum_ok = -1, .bounds_ok = 1,
                        .header_length = 1);
                    walk_cea_block(e, nbi);
                    b += db_size + 1;
                }
                if (b > db_end) {
                    b = db_end;
                    edid_msg(e, "CEA block overrun [in ext " OFMT "], expect to end at +%d, but last ends at +%d" , f, db_end-1, b-1);
                }
                /* DTDs */
                while(b < 127) {
                    if (u8[f+b]) {
                        ADD_BLOCK(e,
                            .type = EDID_BLK_DTD, .first = f+b, .length = 18,
                            .parent = pi, .checksum_ok = -1, .bounds_ok = 1);
                    }
                    b += 18;
                }
            }
            break;
        case EDID_BLK_EXT_DID:
            /* note: f is from the EXT block start, so the DisplayID spec table offsets need +1 */
            //TODO: may actually be longer than 128
            db_end = u8[f+2] + 5;
            b = 5;
            while(b < db_end) {
                if (r24le(e, f + b, NOMASK) == 0) break;
                int db_type = u8[f+b];
                int db_revision = u8[f+b+1] & 0x7;
                int db_size = u8[f+b+2];
                int nbi = e->block_count;
                ADD_BLOCK(e,
                    .type = EDID_BLK_DID, .first = f+b, .length = 3+db_size,
                    .parent = pi, .checksum_ok = -1, .bounds_ok = 1,
                    .header_length = 3);
                walk_did_block(e, nbi);
                b += db_size + 3;
            }
            if (b > db_end)
                edid_msg(e, "DID block overrun [in ext " OFMT "], expect to end at +%d, but last ends at +%d" , f, db_end-1, b-1);
            break;
    }
}

V2_EDID *V2_edid_new(const char *data, unsigned int len) {
    if (len < 128) return NULL;

    int i, j;
    V2_EDID *e = malloc(sizeof(V2_EDID));
    memset(e, 0, sizeof(V2_EDID));
    e->data = malloc(len);
    memcpy(e->data, data, len);
    e->len = len;
    e->msg_log = g_string_new(NULL);

    e->ver_major = e->u8[18];
    e->ver_minor = e->u8[19];

#define RESERVE_COUNT 300
    e->blocks = malloc(sizeof(V2_EDIDBlock) * RESERVE_COUNT);
    memset(e->blocks, 0, sizeof(V2_EDIDBlock) * RESERVE_COUNT);

    ADD_BLOCK(e,
        .type = EDID_BLK_BASE, .first = 0, .length = 128,
        .checksum_ok = block_check(e, 0), .bounds_ok = 1,
        .footer_length = 1);
    ADD_BLOCK(e,
        .type = EDID_BLK_HEADER, .first = 0, .length = 8,
        .parent = 1, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_PROD, .first = 8, .length = 9,
        .parent = 1, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_VENDOR_PNP3, .first = 8, .length = 2,
        .parent = 3, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_PROD_CODE, .first = 10, .length = 2,
        .parent = 3, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_PROD_SERIAL, .first = 12, .length = 4,
        .parent = 3, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_DOM90, .first = 16, .length = 2,
        .parent = 3, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_VERSION, .first = 18, .length = 2,
        .parent = 1, .checksum_ok = -1, .bounds_ok = 1 );
    ADD_BLOCK(e,
        .type = EDID_BLK_ETB, .first = 35, .length = 3,
        .parent = 1, .checksum_ok = -1, .bounds_ok = 1 );
    for(i = 0; i < 8; i++) {
        ADD_BLOCK(e,
            .type = EDID_BLK_STD, .first = 38 + (i*2), .length = 2,
            .parent = 1, .checksum_ok = -1, .bounds_ok = 1 );
    }
    for(i = 54; i < 126; i+=18) {
        if (e->u8[i] != 0) {
            /* DTD */
            ADD_BLOCK(e,
                .type = EDID_BLK_DTD, .first = i, .length = 18,
                .parent = 1, .checksum_ok = -1, .bounds_ok = 1);
            continue;
        }
        int pblk = e->block_count+1;
        ADD_BLOCK(e,
            .type = EDID_BLK_DESC, .first = i, .length = 18,
            .parent = 1, .checksum_ok = -1, .bounds_ok = 1,
            .header_length = 4 );
        switch (e->u8[i+3]) {
            case 0xfc:
                ADD_BLOCK(e,
                    .type = EDID_BLK_STR_NAME, .first = i+5, .length = 13,
                    .parent = pblk, .checksum_ok = -1, .bounds_ok = 1);
                break;
            case 0xff:
                ADD_BLOCK(e,
                    .type = EDID_BLK_STR_SERIAL, .first = i+5, .length = 13,
                    .parent = pblk, .checksum_ok = -1, .bounds_ok = 1);
                break;
            case 0xfe:
                ADD_BLOCK(e,
                    .type = EDID_BLK_STR, .first = i+5, .length = 13,
                    .parent = pblk, .checksum_ok = -1, .bounds_ok = 1);
                break;
            case 0xfa: /* 6 more STDs */
                for(j = 0; j < 6; j++) {
                    ADD_BLOCK(e,
                        .type = EDID_BLK_STD, .first = i + 5 + (j*2), .length = 2,
                        .parent = pblk, .checksum_ok = -1, .bounds_ok = 1 );
                }
                break;
        }
    }

    /* extension blocks */
    int ext = len / 128;
    for(i = 1; i < ext; i++) {
        int ext_type = r8(e, i*128, NOMASK);
        int blk_type = 0;
        int head_size = 1;
        switch (ext_type) {
            case 0x02: /* CEA-EXT */
                blk_type = EDID_BLK_EXT_CEA;
                head_size = 3;
                break;
            case 0x40: /* DI-EXT */
                blk_type = EDID_BLK_EXT_DI;
                head_size = 2;
                break;
            case 0x70: /* DisplayID */
                blk_type = EDID_BLK_EXT_DID;
                head_size = 4;
                break;
            case 0xf0: /* EXT map */
                blk_type = EDID_BLK_EXT_MAP;
                break;
            default:
                blk_type = EDID_BLK_EXT;
        }
        int bi = e->block_count;
        ADD_BLOCK(e,
            .type = blk_type, .first = i*128, .length = 128,
            .parent = 0, .checksum_ok = block_check(e, i*128),
            .bounds_ok = bounds_check(e, (i*128)+127),
            .header_length = head_size, .footer_length = 1 );
        walk_ext_block(e, bi);
    }

    /* squeeze lists */
#define SQUEEZE(C, L) \
    if (!e->C) { free(e->L); e->L = NULL; } \
    else { e->L = realloc(e->L, sizeof(e->L[0]) * (e->C)); }

    SQUEEZE(block_count, blocks);
    return e;
}

void V2_edid_free(V2_EDID *e) {
    int i;
    if (e) {
        g_free(e->blocks);
        g_free(e->data);
        g_string_free(e->msg_log, TRUE);
        g_free(e);
    }
}

V2_EDID *V2_edid_new_from_hex(const char *hex_string) {
    int blen = strlen(hex_string) / 2;
    uint8_t *buffer = malloc(blen), *n = buffer;
    memset(buffer, 0, blen);
    int len = 0;

    const char *p = hex_string;
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

    V2_EDID *e = V2_edid_new((char*)buffer, len);
    free(buffer);
    return e;
}

V2_EDID *V2_edid_new_from_file(const char *path) {
    char *bin = NULL;
    gsize len = 0;
    if (g_file_get_contents(path, &bin, &len, NULL) ) {
        V2_EDID *ret = V2_edid_new(bin, len);
        g_free(bin);
        return ret;
    }
    return NULL;
}

char *V2_edid_dump_hex(V2_EDID *e, int tabs, int breaks) {
    if (!e) return NULL;
    int lines = 1 + (e->len / 16);
    int blen = lines * 35 + 1;
    unsigned int pc = 0;
    char *ret = malloc(blen);
    memset(ret, 0, blen);
    uint8_t *u8 = e->u8;
    char *p = ret;
    for(; lines; lines--) {
        int i, d = MIN(16, (e->len - pc));
        if (!d) break;
        for(i = 0; i < tabs; i++)
            sprintf(p++, "\t");
        for(i = d; i; i--) {
            sprintf(p, "%02x", (unsigned int)*u8);
            p+=2;
            u8++;
            pc++;
            if (pc == e->len) {
                if (breaks) sprintf(p++, "\n");
                goto edid_dump_hex_done;
            }
        }
        if (breaks) sprintf(p++, "\n");
    }
edid_dump_hex_done:
    return ret;
}

#define BTQ(BTE) case BTE: return #BTE;
const char *V2_edid_block_type(int type) {
    switch(type) {
        BTQ( EDID_BLK_INVALID )
        BTQ( EDID_BLK_BASE )
        BTQ( EDID_BLK_HEADER )
        BTQ( EDID_BLK_PROD )
        BTQ( EDID_BLK_VERSION )
        BTQ( EDID_BLK_PROD_CODE )
        BTQ( EDID_BLK_PROD_SERIAL )
        BTQ( EDID_BLK_DESC )
        BTQ( EDID_BLK_VENDOR_PNP3 )
        BTQ( EDID_BLK_VENDOR_PNP4 )
        BTQ( EDID_BLK_VENDOR_OUI )
        BTQ( EDID_BLK_DOM90 )
        BTQ( EDID_BLK_DOM2K )
        BTQ( EDID_BLK_EXT )
        BTQ( EDID_BLK_EXT_MAP )
        BTQ( EDID_BLK_EXT_CEA )
        BTQ( EDID_BLK_EXT_DID )
        BTQ( EDID_BLK_EXT_DI )
        BTQ( EDID_BLK_ETB )
        BTQ( EDID_BLK_STD )
        BTQ( EDID_BLK_DTD )
        BTQ( EDID_BLK_SVD )
        BTQ( EDID_BLK_SAD )
        BTQ( EDID_BLK_DID_T1 )
        BTQ( EDID_BLK_STR )
        BTQ( EDID_BLK_STR_NAME )
        BTQ( EDID_BLK_STR_SERIAL )
        BTQ( EDID_BLK_CEA )
        BTQ( EDID_BLK_DID )
    }
    return "unknown";
}

static char *V2_edid_dump_block_tree(V2_EDID *e, int depth, int parent) {
    char *ret = NULL;
    char bar[256] = "", *p = bar;
    int i;
    for(i = 0; i < depth; i++) {
        strcpy(p, "|  ");
        p = p + strlen(p);
    }

    for(i = 0; i < e->block_count; i++) {
        if (e->blocks[i].parent == parent) {
            gchar *checks = g_strdup("");
            int ok = 1;
            if (!e->blocks[i].checksum_ok) {
                ok = 0;
                checks = appfsp(checks, "%s", "sum_fail!");
            }
            if (!e->blocks[i].bounds_ok) {
                ok = 0;
                checks = appfsp(checks, "%s", "bounds_fail!");
            }
            if (ok) {
                checks = appfsp(checks, "%s", "ok");
            }
            ret = appfnl(ret, "%s+- blk[%d] %s [%d-%d] %s",
                bar, i,
                V2_edid_block_type(e->blocks[i].type),
                e->blocks[i].first, e->blocks[i].first + e->blocks[i].length - 1,
                checks
                );
            gchar *childs = V2_edid_dump_block_tree(e, depth+1, i+1);
            if (childs)
                ret = appfnl(ret, "%s", childs);
            g_free(childs);
        }
    }
    return ret;
}

char *V2_edid_blk_dump(V2_EDIDBlock *blk) {
    gchar *hb_head = NULL, *hb_payload = NULL, *hb_foot = NULL;
    if (blk->header_length) {
        hb_head = hex_bytes(blk->e->u8 + blk->first, blk->header_length);
    } else
        hb_head = g_strdup("");

    hb_payload = hex_bytes(
        blk->e->u8 + blk->first + blk->header_length,
        blk->length - blk->header_length - blk->footer_length);

    if (blk->footer_length) {
        hb_foot = hex_bytes(
            blk->e->u8 + blk->first + blk->length - blk->footer_length,
            blk->footer_length);
    } else
        hb_foot = g_strdup("");

    gchar *ret = g_strdup_printf("%s|%s|%s",
        g_strstrip(hb_head), g_strstrip(hb_payload), g_strstrip(hb_foot) );

    g_free(hb_head);
    g_free(hb_payload);
    g_free(hb_foot);
    return ret;
}

char *V2_edid_dump(V2_EDID *e) {
    char *ret = NULL;
    int i;
    if (!e) return NULL;

    ret = appfnl(ret, "edid version: %d.%d (%d bytes)", e->ver_major, e->ver_minor, e->len);
    if (e->extends_to)
        ret = appfnl(ret, "extended to: %s", _(edid_standard(e->extends_to)) );

    for(i = 0; i < e->block_count; i++) {
        char *hb = V2_edid_blk_dump(&e->blocks[i]);
        ret = appfnl(ret, "blk[%d]: parent:[%d], type:%02x, length:%d, range:%d-%d, checksum_ok:%d, bounds_ok:%d -- %s",
                i, e->blocks[i].parent ? e->blocks[i].parent-1 : -1,
                e->blocks[i].type, e->blocks[i].length,
                e->blocks[i].first, e->blocks[i].first + e->blocks[i].length - 1,
                e->blocks[i].checksum_ok, e->blocks[i].bounds_ok,
                hb);
        free(hb);
    }
    gchar *childs = V2_edid_dump_block_tree(e, 0, 0);
    if (childs)
        ret = appfnl(ret, "%s", childs);

    return ret;
}


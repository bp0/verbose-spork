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

#include "util_dt.h"
#include "format_funcs.h"

void dtr_msg(char *fmt, ...);
int dtr_guess_type(sysobj *obj);

enum {
    DT_TYPE_ERR,

    DT_NODE,
    DTP_UNK,     /* arbitrary-length byte sequence */
    DTP_EMPTY,   /* empty property */
    DTP_STR,     /* null-delimited list of strings */
    DTP_COMPAT,  /* "compatible", DTP_STR */
    DTP_HEX,     /* list of 32-bit values displayed in hex */
    DTP_UINT,    /* unsigned int list */
    DTP_UINT64,  /* unsigned int64 list */
    DTP_INTRUPT, /* interrupt-specifier list */
    DTP_INTRUPT_EX, /* extended interrupt-specifier list */
    DTP_OVR,     /* all in /__overrides__ */
    DTP_PH,      /* phandle */
    DTP_PH_REF,  /* reference to phandle */
    DTP_REG,     /* <#address-cells, #size-cells> */
    DTP_CLOCKS,  /* <phref, #clock-cells> */
    DTP_GPIOS,   /* <phref, #gpio-cells> */
    DTP_DMAS,    /* dma-specifier list */
    DTP_OPP1,    /* list of operating points, pairs of (kHz,uV) */
    DTP_PH_REF_OPP2,  /* phandle reference to opp-v2 table */
};

gchar* dtr_log = NULL;
static GSList *phandles = NULL;
static GSList *aliases = NULL;
static GSList *symbols = NULL;

void dtr_msg(char *fmt, ...) {
    gchar *buf, *tmp;
    va_list args;

    va_start(args, fmt);
    buf = g_strdup_vprintf(fmt, args);
    va_end(args);

    tmp = g_strdup_printf("%s%s\n", dtr_log, buf);
    g_free(dtr_log);
    dtr_log = tmp;
}

typedef struct {
    uint32_t v;  /* phandle */
    gchar *label;  /* alias */
    gchar *path;
} dtr_map_item;

static void dtr_map_free(dtr_map_item *s) {
    if (s) {
        g_free(s->label);
        g_free(s->path);
        g_free(s);
    }
}

static struct {
    gchar *pattern;
    int type;
    GPatternSpec *pspec; /* compiled later */
} prop_types[] = {
    { "name", DTP_STR, NULL },
    { "compatible", DTP_COMPAT, NULL },
    { "model", DTP_STR, NULL },
    { "reg", DTP_REG, NULL },
    { "clocks", DTP_CLOCKS, NULL },
    { "gpios", DTP_GPIOS, NULL },
    { "cs-gpios", DTP_GPIOS, NULL },
    { "phandle", DTP_PH, NULL },
    { "interrupts", DTP_INTRUPT, NULL },
    { "interrupts-extended", DTP_INTRUPT_EX, NULL },
    { "interrupt-parent", DTP_PH_REF, NULL },
    { "interrupt-controller", DTP_EMPTY, NULL },
    { "regulator-min-microvolt", DTP_UINT, NULL },
    { "regulator-max-microvolt", DTP_UINT, NULL },
    { "clock-frequency", DTP_UINT, NULL },
    { "dmas", DTP_DMAS, NULL },
    { "dma-channels", DTP_UINT, NULL },
    { "dma-requests", DTP_UINT, NULL },
    /* operating-points-v2: */
    /* https://www.kernel.org/doc/Documentation/devicetree/bindings/opp/opp.txt */
    { "operating-points", DTP_OPP1, NULL },
    { "operating-points-v2", DTP_PH_REF_OPP2, NULL },
    { "opp-hz", DTP_UINT64, NULL },
    { "opp-microvolt", DTP_UINT, NULL },
    { "opp-microvolt-*", DTP_UINT, NULL }, /* opp-microvolt-<named> */
    { "opp-microamp", DTP_UINT, NULL },
    { "clock-latency-ns", DTP_UINT, NULL },
    { "max-speed", DTP_UINT, NULL },
};

static struct {
    char *name; uint32_t v;
} default_values[] = {
    { "#address-cells", 2 },
    { "#size-cells", 1 },
    { "#dma-cells", 1 },
    { NULL, 0 },
};

/* find the value of a path-inherited property by climbing the path */
int dtr_inh_find(sysobj *obj, char *qprop, int limit) {
    sysobj *tobj = NULL, *pobj = NULL, *qobj = NULL;
    uint32_t ret = 0;
    int i, found = 0;

    if (!limit) limit = 100;

    tobj = obj;
    while (tobj != NULL) {
        pobj = sysobj_parent(tobj, FALSE);
        if (tobj != obj)
            sysobj_free(tobj);
        if (!limit || pobj == NULL) break;
        qobj = sysobj_sibling(pobj, qprop, FALSE);
        if (qobj != NULL && qobj->exists) {
            sysobj_read(qobj, FALSE);
            ret = be32toh(*qobj->data.uint32);
            found = 1;
            sysobj_free(qobj);
            break;
        }
        sysobj_free(qobj);
        tobj = pobj;
        limit--;
    }
    sysobj_free(pobj);

    if (!found) {
        i = 0;
        while(default_values[i].name != NULL) {
            if (SEQ(default_values[i].name, qprop)) {
                ret = default_values[i].v;
                dtr_msg("Using default value %d for %s in %s\n", ret, qprop, obj->path);
                break;
            }
            i++;
        }
    }

    return ret;
}

char *dtr_list_byte(uint8_t *bytes, gsize count) {
    uint32_t v;
    gchar buff[4] = "";  /* max element: " 00\0" */
    gsize i, blen = count * 4 + 1;
    gchar *ret = g_new0(gchar, blen);
    strcpy(ret, "[");
    for (i = 0; i < count; i++) {
        v = bytes[i];
        sprintf(buff, "%s%02x", i ? " " : "", v);
        g_strlcat(ret, buff, blen);
    }
    g_strlcat(ret, "]", blen);
    return ret;
}

char *dtr_list_hex(dt_uint *list, gsize count) {
    gchar buff[12] = "";  /* max element: " 0x00000000\0" */
    gsize i, blen = count * 12 + 1;
    gchar *ret = g_new0(gchar, blen);
    for (i = 0; i < count; i++) {
        sprintf(buff, "%s0x%x", i ? " " : "", be32toh(list[i]) );
        g_strlcat(ret, buff, blen);
    }
    return ret;
}

char *dtr_list_uint32(dt_uint *list, gsize count) {
    char buff[12] = "";  /* max element: " 4294967296\0" */
    gsize i, blen = count * 12 + 1;
    gchar *ret = g_new0(gchar, blen);
    for (i = 0; i < count; i++) {
        sprintf(buff, "%s%u", i ? " " : "", be32toh(list[i]) );
        g_strlcat(ret, buff, blen);
    }
    return ret;
}

char *dtr_list_uint64(dt_uint64 *list, gsize count) {
    char buff[24] = "";  /* max element: " <2^64>\0" */
    gsize i, blen = count * 24 + 1;
    gchar *ret = g_new0(gchar, blen);
    for (i = 0; i < count; i++) {
        sprintf(buff, "%s%" PRIu64, i ? " " : "", be64toh(list[i]) );
        g_strlcat(ret, buff, blen);
    }
    return ret;
}

gchar *dtr_list_str0(const char *data, gsize length) {
    char *tmp, *esc, *next_str;
    char ret[1024] = "";
    gsize l, tl;

    /* treat as null-separated string list */
    tl = 0;
    tmp = ret;
    next_str = (char*)data;
    while(next_str != NULL) {
        l = strlen(next_str);
        if (tl + l >= 1020)
            break;
        esc = g_strescape(next_str, NULL);
        sprintf(tmp, "%s\"%s\"",
                strlen(ret) ? ", " : "", esc);
        free(esc);
        tmp += strlen(tmp);
        tl += l + 1; next_str += l + 1;
        if (tl >= length) break;
    }
    return strdup(ret);
}

char *dtr_list_reg(sysobj *obj) {
    gchar *tup_str, *ret = NULL;
    uint32_t acells, scells, tup_len;
    uint32_t extra, consumed; /* bytes */
    uint32_t *next;

    acells = dtr_inh_find(obj, "#address-cells", 2);
    scells = dtr_inh_find(obj, "#size-cells", 2);
    tup_len = acells + scells;
    extra = obj->data.len % (tup_len * 4);
    consumed = 0; /* bytes */

    //printf("list reg: %s\n ... acells: %u, scells: %u, extra: %u\n", obj->path, acells, scells, extra);

    if (extra) {
        /* error: length is not a multiple of tuples */
        dtr_msg("Data length (%u) is not a multiple of (#address-cells:%u + #size-cells:%u) for %s\n",
            obj->data.len, acells, scells, obj->path);
        return dtr_list_hex(obj->data.uint32, obj->data.len / 4);
    }

    next = obj->data.uint32;
    consumed = 0;
    while (consumed + (tup_len * 4) <= obj->data.len) {
        tup_str = dtr_list_hex(next, tup_len);
        ret = appfsp(ret, "<%s>", tup_str);
        g_free(tup_str);
        consumed += (tup_len * 4);
        next += tup_len;
    }

    //printf(" ... %s\n", ret);
    return ret;
}

uint32_t dtr_get_phref_prop(uint32_t phandle, gchar *prop) {
    uint32_t ret = 0;
    sysobj *obj = sysobj_new_from_fn(dtr_phandle_lookup(phandle), prop);
    if (obj && obj->exists) {
        sysobj_read(obj, FALSE);
        ret = be32toh(*obj->data.uint32);
    }
    sysobj_free(obj);
    return ret;
}

gchar *dtr_elem_hex(dt_uint e) {
    return g_strdup_printf("0x%x", be32toh(e) );
}

char *dtr_elem_phref(dt_uint e, int show_path, const gchar *extra) {
    const gchar *ph_path = NULL, *ph_path_trim = NULL, *al_label = NULL;
    char *ret = NULL;
    ph_path = dtr_phandle_lookup(be32toh(e));
    if (ph_path != NULL) {
        if (show_path)
            ph_path_trim = ph_path + strlen(DTROOT);

        /* TODO: alias or symbol? */
        al_label = dtr_symbol_lookup_by_path(ph_path);
        if (al_label != NULL) {
            if (show_path)
                ret = g_strdup_printf("&%s (%s) %s", al_label, ph_path_trim, extra ? extra : "");
            else
                ret = g_strdup_printf("&%s %s", al_label, extra ? extra : "");
        } else {
            if (show_path)
                ret = g_strdup_printf("0x%x (%s) %s", be32toh(e), ph_path_trim, extra ? extra : "");
        }
    }
    if (ret == NULL)
        ret = dtr_elem_hex(e);
    return ret;
}

char *dtr_list_phref(sysobj *obj, gchar *ext_cell_prop) {
    /* <phref, #XXX-cells> */
    int count = obj->data.len / 4;
    int i = 0, ext_cells = 0;
    gchar *ph, *ext, *ret = NULL;

    while (i < count) {
        if (ext_cell_prop == NULL)
            ext_cells = 0;
        else
            ext_cells = dtr_get_phref_prop(be32toh(obj->data.uint32[i]), ext_cell_prop);
        ph = dtr_elem_phref(obj->data.uint32[i], 0, NULL); i++;
        if (ext_cells > count - i) ext_cells = count - i;
        ext = dtr_list_hex((obj->data.uint32 + i), ext_cells); i+=ext_cells;
        ret = appfsp(ret, "<%s%s%s>",
            ph, (ext_cells) ? " " : "", ext);
        g_free(ph);
        g_free(ext);
    }
    return ret;
}

gchar *dtr_list_interrupts(sysobj *obj) {
    gchar *ext, *ret = NULL;
    uint32_t iparent, icells;
    int count, i = 0;

    iparent = dtr_inh_find(obj, "interrupt-parent", 0);
    if (!iparent) {
        dtr_msg("Did not find an interrupt-parent for %s", obj->path);
        goto intr_err;
    }
    icells = dtr_get_phref_prop(iparent, "#interrupt-cells");
    if (!icells) {
        dtr_msg("Invalid #interrupt-cells value %d for %s", icells, obj->path);
        goto intr_err;
    }

    count = obj->data.len / 4;
    while (i < count) {
        icells = UMIN(icells, count - i);
        ext = dtr_list_hex((obj->data.uint32 + i), icells); i+=icells;
        ret = appfsp(ret, "<%s>", ext);
        g_free(ext);
    }
    return ret;

intr_err:
    return dtr_list_hex(obj->data.uint32, obj->data.len / 4);

}

char *dtr_list_override(sysobj *obj) {
    /* <phref, string "property:value"> ... */
    gchar *ret = NULL;
    gchar *ph, *str;
    gchar *src;
    uint32_t consumed = 0;
    int l;
    src = obj->data.str;
    while (consumed + 5 <= obj->data.len) {
        ph = dtr_elem_phref(*(dt_uint*)src, 1, NULL);
        src += 4; consumed += 4;
        l = strlen(src) + 1; /* consume the null */
        str = dtr_list_str0(src, l);
        ret = appfsp(ret, "<%s -> %s>", ph, str);
        src += l; consumed += l;
        g_free(ph);
        g_free(str);
    }
    if (consumed < obj->data.len) {
        str = dtr_list_byte((uint8_t*)src, obj->data.len - consumed);
        ret = appfsp(ret, "%s", str);
        g_free(str);
    }
    return ret;
}

uint32_t dtr_get_prop_u32(sysobj *node, const char *name) {
    uint32_t ret = 0;
    sysobj *obj = sysobj_new_from_fn(node->path, name);
    if (obj && obj->exists) {
        sysobj_read(obj, FALSE);
        if (obj->data.uint32 != NULL)
            ret = be32toh(*obj->data.uint32);
    }
    sysobj_free(obj);
    return ret;
}

uint64_t dtr_get_prop_u64(sysobj *node, const char *name) {
    uint64_t ret = 0;
    sysobj *obj = sysobj_new_from_fn(node->path, name);
    if (obj && obj->exists) {
        sysobj_read(obj, FALSE);
        if (obj->data.int64 != NULL)
            ret = be64toh(*obj->data.int64);
    }
    sysobj_free(obj);
    return ret;
}

char *dtr_get_prop_str(sysobj *node, const char *name) {
    char *ret = NULL;
    sysobj *obj = sysobj_new_from_fn(node->path, name);
    if (obj && obj->exists) {
        sysobj_read(obj, FALSE);
        if (obj->data.str != NULL)
            ret = g_strdup(obj->data.str);
    }
    sysobj_free(obj);
    return ret;
}

/* export */
gchar *dtr_get_opp_kv(const gchar *path, const gchar *key_prefix) {
    dt_opp_range *opp = dtr_get_opp_range(path);
    if (opp) {
        const gchar *pfx = key_prefix ? key_prefix : "opp.";
        gchar *ret = NULL;
        switch(opp->version) {
            case 1:
            case 2:
                ret = g_strdup_printf(
                    "%sref=%s\n"
                    "%sversion=%" PRId32 "\n"
                    "%skhz_min=%" PRId32 "\n"
                    "%skhz_max=%" PRId32 "\n"
                    "%sclock_latency_ns=%" PRId32 "\n",
                    pfx, opp->ref_path,
                    pfx, opp->version,
                    pfx, opp->khz_min,
                    pfx, opp->khz_max,
                    pfx, opp->clock_latency_ns);
                break;
            case 0:
            default:
                if (opp->clock_latency_ns) {
                    ret = g_strdup_printf(
                        "%sref=%s\n"
                        "%sclock_frequency=%" PRId32 "\n"
                        "%sclock_latency_ns=%" PRId32 "\n",
                        pfx, opp->ref_path,
                        pfx, opp->khz_max,
                        pfx, opp->clock_latency_ns);
                } else {
                    ret = g_strdup_printf(
                        "%sref=%s\n"
                        "%sclock_frequency=%" PRId32 "\n",
                        pfx, opp->ref_path,
                        pfx, opp->khz_max );
                }
                break;
        }
        g_free(opp);
        return ret;
    }
    return NULL;
}

dt_opp_range *dtr_get_opp_range(const gchar *path) {
    dt_opp_range *ret = NULL;
    sysobj *obj = NULL, *table_obj = NULL, *row_obj = NULL;
    uint32_t opp_ph = 0;
    const gchar *opp_table_path = NULL;
    char *tab_compat = NULL, *tab_status = NULL;
    gchar *fn;
    GSList *childs = NULL;
    uint64_t khz = 0;
    uint32_t lns = 0;
    gchar *row_status = NULL;
    uint32_t i = 0;

    obj = sysobj_new_fast(path);
    if (!(obj && obj->exists) )
        goto get_opp_finish;

    opp_ph = dtr_get_prop_u32(obj, "operating-points-v2"); /* OPPv2 */
    table_obj = sysobj_child(obj, "operating-points"); /* OPPv1 */
    if (!opp_ph) {
        if (table_obj->exists) {
            /* only v1 */
            ret = g_new0(dt_opp_range, 1);
            strncpy(ret->ref_path, table_obj->path + sizeof(DTROOT), 127);
            ret->version = 1;
            ret->clock_latency_ns = dtr_get_prop_u32(obj, "clock-latency");

            /* pairs of (kHz,uV) */
            for (i = 0; i < table_obj->data.len; i += 2) {
                khz = table_obj->data.uint32[i];
                if (khz > ret->khz_max)
                    ret->khz_max = khz;
                if (khz < ret->khz_min || ret->khz_min == 0)
                    ret->khz_min = khz;
            }

        } else {
            /* is clock-frequency available? */
            khz = dtr_get_prop_u32(obj, "clock-frequency");
            if (khz) {
                ret = g_new0(dt_opp_range, 1);
                strncpy(ret->ref_path, obj->path + sizeof(DTROOT), 127 - strlen("/clock-frequency") );
                strcat(ret->ref_path, "/clock-frequency");
                ret->version = 0;
                ret->khz_max = khz;
                ret->clock_latency_ns = dtr_get_prop_u32(obj, "clock-latency");
            }
        }
        goto get_opp_finish;
    } else {
        /* use v2 if both available */
        sysobj_free(table_obj);
        table_obj = NULL;
    }

    opp_table_path = dtr_phandle_lookup(opp_ph);
    if (!opp_table_path)
        goto get_opp_finish;
    table_obj = sysobj_new_fast(opp_table_path);
    if (!table_obj)
        goto get_opp_finish;
    tab_compat = dtr_get_prop_str(table_obj, "compatible");
    tab_status = dtr_get_prop_str(table_obj, "status");
    if (!tab_compat || strcmp(tab_compat, "operating-points-v2") != 0)
        goto get_opp_finish;
    if (tab_status && SEQ(tab_status, "disabled"))
        goto get_opp_finish;

    ret = g_new0(dt_opp_range, 1);
    strncpy(ret->ref_path, table_obj->path + sizeof(DTROOT), 127);
    ret->version = 2;
    ret->phandle = opp_ph;

    childs = sysobj_children(table_obj, NULL, NULL, FALSE);
    GSList *l = childs;
    while(l) {
        fn = (gchar*)l->data;
        row_obj = sysobj_child(table_obj, fn);
        if ( row_obj->data.is_dir ) {
            row_status = dtr_get_prop_str(row_obj, "status");
            if (!row_status || strcmp(row_status, "disabled") != 0) {
                khz = dtr_get_prop_u64(row_obj, "opp-hz");
                khz /= 1000; /* 64b hz -> 32b khz */
                lns = dtr_get_prop_u32(row_obj, "clock-latency-ns");
                if (khz > ret->khz_max)
                    ret->khz_max = khz;
                if (khz < ret->khz_min || ret->khz_min == 0)
                    ret->khz_min = khz;
                ret->clock_latency_ns = lns;
            }
            g_free(row_status); row_status = NULL;
        }
        sysobj_free(row_obj); row_obj = NULL;

        /* won't need again */
        g_free(l->data);

        l = l->next;
    }
    g_slist_free(childs);

get_opp_finish:
    sysobj_free(obj);
    sysobj_free(table_obj);
    g_free(tab_status);
    g_free(tab_compat);
    g_free(row_status);
    return ret;
}

char *dtr_elem_oppv2(sysobj* obj) {
    gchar opp_str[512] = "";
    gchar *pp = sysobj_parent_path(obj);
    if (pp) {
        dt_opp_range *opp = dtr_get_opp_range(pp);
        if (opp) {
            snprintf(opp_str, 511, "[%d - %d %s]", opp->khz_min, opp->khz_max, _("kHz"));
            g_free(opp);
        }
        g_free(pp);
    }
    return dtr_elem_phref(*obj->data.uint32, 1, opp_str);
}

int dtr_guess_type_by_name(const gchar *name) {
    gboolean match = FALSE;
    gsize len = strlen(name);
    int i = 0;
    for (i = 0; i < (int)G_N_ELEMENTS(prop_types); i++) {
        if (prop_types[i].pspec)
            match = g_pattern_match(prop_types[i].pspec, len, name, NULL);
        else
            match = (g_strcmp0(name, prop_types[i].pattern) == 0);
        if (match)
            return prop_types[i].type;
    }
    return DTP_UNK;
}

int dtr_guess_type(sysobj *obj) {
    char *tmp, *dash;
    gsize i = 0, anc = 0;
    gboolean might_be_str = TRUE;

    if (obj->data.is_dir)
        return DT_NODE;

    if (obj->data.len == 0)
        return DTP_EMPTY;

    /* special #(.*)-cells names are UINT */
    if (*obj->name == '#') {
        dash = strrchr(obj->name, '-');
        if (dash != NULL) {
            if (SEQ(dash, "-cells"))
                return DTP_UINT;
        }
    }

    gchar *parent = sysobj_parent_name(obj);
    /* /aliases/ * and /__symbols__/ * are always strings */
    if (SEQ(parent, "aliases")
        || SEQ(parent, "__symbols__") ) {
        g_free(parent);
        return DTP_STR;
    }
    /* /__overrides__/ * */
    if (SEQ(parent, "__overrides__")
        && strcmp(obj->name, "name") != 0 ) {
            g_free(parent);
            return DTP_OVR;
        }
    g_free(parent); parent = NULL;

    /* lookup known type */
    int nt = dtr_guess_type_by_name(obj->name);
    if (nt != DTP_UNK) return nt;

    /* maybe a string? */
    if (obj->data.is_utf8)
        return DTP_STR;
    for (i = 0; i < obj->data.len; i++) {
        tmp = obj->data.str + i;
        if ( isalnum(*tmp) ) anc++; /* count the alpha-nums */
        if ( isprint(*tmp) || *tmp == 0 )
            continue;
        might_be_str = FALSE;
        break;
    }
    if (might_be_str &&
        ( anc >= obj->data.len - 2 /* all alpha-nums but ^/ and \0$ */
          || anc >= 5 ) /*arbitrary*/)
        return DTP_STR;

    /* multiple of 4 bytes, try list of hex values */
    if (!(obj->data.len % 4))
        return DTP_HEX;

    return DTP_UNK;
}

/* export */
gchar *dtr_compat_decode(const gchar *compat_str_list, gsize len, gboolean show_class, int fmt_opts) {
    gchar *ret = NULL;
    if (compat_str_list) {
        const gchar *el = compat_str_list;
        while(el < compat_str_list + len) {
            gchar *lookup_path = g_strdup_printf(":/lookup/dt.ids/%s", el);
            gchar *cls = show_class ? sysobj_raw_from_fn(lookup_path, "class") : NULL;
            gchar *vendor = sysobj_raw_from_fn(lookup_path, "vendor");
            gchar *name = sysobj_raw_from_fn(lookup_path, "name");

            gchar *vtag = vendor_match_tag(vendor, fmt_opts);
            if (vtag) {
                g_free(vendor);
                vendor = vtag;
            }

            if (vendor && name && cls)
                ret = appf(ret, ", ", "%s %s (%s)", vendor, name, cls);
            else if (name && cls)
                ret = appf(ret, ", ", "%s (%s)", name, cls);
            else if (vendor && name)
                ret = appf(ret, ", ", "%s %s", vendor, name);
            else if (vendor)
                ret = appf(ret, ", ", "%s", vendor);
            else if (name)
                ret = appf(ret, ", ", "%s", name);

            g_free(vendor);
            g_free(name);
            g_free(cls);
            g_free(lookup_path);
            el += strlen(el) + 1;
        }
    }
    return ret;
}

gchar *dtr_format(sysobj *obj, int fmt_opts) {
    gchar *ret = NULL;
    const gchar *sym, *al;
    int type = dtr_guess_type(obj);
    gboolean needs_escape = TRUE;
    switch(type) {
        case DT_NODE:
            sym = dtr_symbol_lookup_by_path(obj->path);
            al = dtr_alias_lookup_by_path(obj->path);
            if (sym || al)  {
                gchar *tlist = g_strdup("{dt node}");
                if (sym) tlist = appfsp(tlist, "symbol:%s", sym);
                if (al) tlist = appfsp(tlist, "alias:%s", al);
                return tlist;
            }
            return g_strdup("{dt node}");
        case DTP_EMPTY:
            if (fmt_opts & FMT_OPT_NULL_IF_EMPTY)
                return NULL;
            return g_strdup("{empty}");
        case DTP_COMPAT:
            if (fmt_opts & FMT_OPT_NULL_IF_EMPTY) {
                if (obj->data.len == 1 && *obj->data.str == 0)
                    return NULL;
            }
            gchar *compat = dtr_compat_decode(obj->data.str, obj->data.len, TRUE, fmt_opts);
            gchar *raw = dtr_list_str0(obj->data.str, obj->data.len);
            if (compat)
                ret = g_strdup_printf("[%s] %s", raw, compat);
            else
                ret = g_strdup_printf("[%s]", raw);
            g_free(compat);
            g_free(raw);
            needs_escape = FALSE;
            break;
        case DTP_STR:
            if (fmt_opts & FMT_OPT_NULL_IF_EMPTY) {
                if (obj->data.len == 1 && *obj->data.str == 0)
                    return NULL;
            }
            ret = dtr_list_str0(obj->data.str, obj->data.len);
            break;
        case DTP_OPP1:
        case DTP_UINT:
            ret = dtr_list_uint32(obj->data.uint32, obj->data.len / 4);
            break;
        case DTP_UINT64:
            ret = dtr_list_uint64(obj->data.uint64, obj->data.len / 8);
            break;
        case DTP_OVR:
            ret = dtr_list_override(obj);
            break;
        case DTP_REG:
            /* <#address-cells #size-cells> */
            ret = dtr_list_reg(obj);
            break;
        case DTP_INTRUPT:
            ret = dtr_list_interrupts(obj);
            break;
        case DTP_INTRUPT_EX:
            /* <phref, #interrupt-cells"> */
            ret = dtr_list_phref(obj, "#interrupt-cells");
            break;
        case DTP_CLOCKS:
            /* <phref, #clock-cells"> */
            ret = dtr_list_phref(obj, "#clock-cells");
            break;
        case DTP_GPIOS:
            /* <phref, #gpio-cells"> */
            ret = dtr_list_phref(obj, "#gpio-cells");
            break;
        case DTP_DMAS:
            /* <phref, #dma-cells"> */
            ret = dtr_list_phref(obj, "#dma-cells");
            break;
        case DTP_PH:
        case DTP_HEX:
            if (obj->data.len % 4)
                ret = dtr_list_byte((uint8_t*)obj->data.any, obj->data.len);
            else
                ret = dtr_list_hex(obj->data.any, obj->data.len / 4);
            break;
        case DTP_PH_REF:
            ret = dtr_elem_phref(*obj->data.uint32, 1, NULL);
            break;
        case DTP_PH_REF_OPP2:
            ret = dtr_elem_oppv2(obj);
            break;
        case DTP_UNK:
        default:
            if (obj->data.len > 64) /* maybe should use #define at the top */
                ret = g_strdup_printf("{data} (%lu bytes)", obj->data.len);
            else
                ret = dtr_list_byte((uint8_t*)obj->data.any, obj->data.len);
            break;
    }
    if (ret) {
        if (needs_escape &&
            (fmt_opts & FMT_OPT_PANGO || fmt_opts & FMT_OPT_HTML) )
            ret = util_escape_markup(ret, TRUE);
        return ret;
    }

    return simple_format(obj, fmt_opts);
}

const char *dtr_phandle_lookup(uint32_t v) {
    /* 0 and 0xffffffff are invalid phandle values */
    /* TODO: perhaps "INVALID" or something */
    if (v == 0 || v == 0xffffffff)
        return NULL;
    GSList *l = phandles;
    while(l) {
        dtr_map_item *mi = l->data;
        if (mi->v == v)
            return mi->path;
        l = l->next;
    }
    return NULL;
}

const char *dtr_alias_lookup(const gchar* label) {
    GSList *l = aliases;
    while(l) {
        dtr_map_item *ali = (dtr_map_item *)l->data;
        if (SEQ(ali->label, label))
            return ali->path;
        l = l->next;
    }
    return NULL;
}

const char *dtr_alias_lookup_by_path(const gchar* path) {
    GSList *l = aliases;
    while(l) {
        dtr_map_item *ali = (dtr_map_item *)l->data;
        if (SEQ(ali->path, path))
            return ali->label;
        l = l->next;
    }
    return NULL;
}

static void dtr_alias_scan() {
    sysobj *alo = sysobj_new_from_fn(DTROOT, "aliases");
    GSList *childs = sysobj_children(alo, NULL, "name", FALSE);
    GSList *l = childs;
    while(l) {
        gchar *fn = (gchar *)l->data;
        gchar *apath = dtr_get_prop_str(alo, fn);
        sysobj *target = sysobj_new_from_fn(DTROOT, apath);
        if (apath) {
            dtr_map_item *nmi = g_new0(dtr_map_item, 1);
            nmi->label = fn;
            nmi->path = g_strdup(target->path);
            aliases = g_slist_append(aliases, nmi);
            sysobj_virt_add_simple(":/devicetree/_alias_map", fn, nmi->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
        }
        sysobj_free(target);
        g_free(apath);
        l = l->next;
    }
    /* the aliases list owns each fn (->data) via nmi */
    g_slist_free(childs);
    sysobj_free(alo);
}

const char *dtr_symbol_lookup_by_path(const gchar* path) {
    GSList *l = symbols;
    while(l) {
        dtr_map_item *ali = (dtr_map_item *)l->data;
        if (SEQ(ali->path, path))
            return ali->label;
        l = l->next;
    }
    return NULL;
}

static void dtr_symbol_scan() {
    sysobj *alo = sysobj_new_from_fn(DTROOT, "__symbols__");
    GSList *childs = sysobj_children(alo, NULL, "name", FALSE);
    GSList *l = childs;
    while(l) {
        gchar *fn = (gchar *)l->data;
        gchar *apath = dtr_get_prop_str(alo, fn);
        sysobj *target = sysobj_new_from_fn(DTROOT, apath);
        if (apath) {
            dtr_map_item *nmi = g_new0(dtr_map_item, 1);
            nmi->label = fn;
            nmi->path = g_strdup(target->path);
            symbols = g_slist_append(symbols, nmi);
            sysobj_virt_add_simple(":/devicetree/_symbol_map", fn, nmi->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
        }
        sysobj_free(target);
        g_free(apath);
        l = l->next;
    }
    /* the symbols list owns each fn (->data) via nmi */
    g_slist_free(childs);
    sysobj_free(alo);
}

static void dtr_phandle_scan(gchar *nb, gchar *nn) {
    gchar phstr[20] = "";
    sysobj *obj = sysobj_new_from_fn(nb, nn);
    if (obj && obj->exists && obj->data.is_dir) {
        /* this object */
        sysobj *phobj = sysobj_child(obj, "phandle");
        if (phobj && phobj->exists) {
            sysobj_read(phobj, FALSE);
            dtr_map_item *nmi = g_new0(dtr_map_item, 1);
            nmi->v = be32toh(*phobj->data.uint32);
            nmi->path = g_strdup(obj->path);
            phandles = g_slist_append(phandles, nmi);
            sprintf(phstr, "0x%08x", nmi->v);
            sysobj_virt_add_simple(":/devicetree/_phandle_map", phstr, nmi->path, VSO_TYPE_SYMLINK | VSO_TYPE_AUTOLINK | VSO_TYPE_DYN );
        }
        sysobj_free(phobj);

        /* scan children */
        GSList *childs = sysobj_children(obj, NULL, NULL, FALSE);
        GSList *l = childs;
        while(l) {
            dtr_phandle_scan(obj->path, l->data);
            /* won't need this again */
            g_free(l->data);
            l = l->next;
        }
        g_slist_free(childs);
    }
    sysobj_free(obj);
}

gboolean dtr_init() {
    /* compile type table patterns */
    for (int i = 0; i < (int)G_N_ELEMENTS(prop_types); i++) {
        if ( strchr(prop_types[i].pattern,'*')
            || strchr(prop_types[i].pattern,'?') )
            prop_types[i].pspec = g_pattern_spec_new(prop_types[i].pattern);
    }

    dtr_log = g_strdup("");

    if (!sysobj_exists_from_fn(DTROOT, NULL)) {
        dtr_msg("devicetree not found at %s", DTROOT);
    } else {
        dtr_alias_scan();
        dtr_msg("%d alias(es) read from /aliases.", g_slist_length(aliases) );
        dtr_symbol_scan();
        dtr_msg("%d symbol(s) read from /__symbols__.", g_slist_length(symbols) );
        dtr_phandle_scan(DTROOT, NULL);
        dtr_msg("%d phandle(s) found.", g_slist_length(phandles) );
    }
}

void dtr_cleanup() {
    for (int i = 0; i < (int)G_N_ELEMENTS(prop_types); i++) {
        if (prop_types[i].pspec)
            g_pattern_spec_free(prop_types[i].pspec);
    }
    if (phandles)
        g_slist_free_full(phandles, (GDestroyNotify)dtr_map_free);
    if (aliases)
        g_slist_free_full(aliases, (GDestroyNotify)dtr_map_free);
    if (symbols)
        g_slist_free_full(symbols, (GDestroyNotify)dtr_map_free);
    g_free(dtr_log);
}


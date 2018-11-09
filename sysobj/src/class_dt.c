
#include "sysobj.h"

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h> /* for PRIu64 */
#include <endian.h>

#define DTROOT "/sys/firmware/devicetree/base"

typedef uint32_t dt_uint; /* big-endian */
typedef uint64_t dt_uint64; /* big-endian */

gchar* dtr_log = NULL;

static gchar *dtr_format(sysobj *obj, int fmt_opts);
static guint dtr_flags(sysobj *obj);
static gchar *dt_messages(const gchar *path);
static double dtr_update_interval(sysobj *obj) { PARAM_NOT_UNUSED(obj); return 0.0; } /* dt is static */

void class_dt_cleanup();

enum {
    DT_TYPE_ERR,

    DT_NODE,
    DTP_UNK,     /* arbitrary-length byte sequence */
    DTP_EMPTY,   /* empty property */
    DTP_STR,     /* null-delimited list of strings */
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

#define CLS_DT_FLAGS OF_GLOB_PATTERN | OF_CONST

static sysobj_class cls_dtr[] = {
  { .tag = "devicetree/compat", .pattern = DTROOT "*/compatible", .flags = CLS_DT_FLAGS,
    .f_format = dtr_format, .f_update_interval = dtr_update_interval },
  /* all else */
  { .tag = "devicetree", .pattern = DTROOT "*", .flags = CLS_DT_FLAGS,
    .f_format = dtr_format, .f_flags = dtr_flags, .f_update_interval = dtr_update_interval, .f_cleanup = class_dt_cleanup },
};

static sysobj_virt vol[] = {
    { .path = ":devicetree", .str = "*",
      .type = VSO_TYPE_DIR | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":devicetree/base", .str = DTROOT,
      .type = VSO_TYPE_AUTOLINK | VSO_TYPE_SYMLINK | VSO_TYPE_DYN | VSO_TYPE_CONST,
      .f_get_data = NULL, .f_get_type = NULL },
    { .path = ":devicetree/_messages", .str = "",
      .type = VSO_TYPE_STRING | VSO_TYPE_CONST,
      .f_get_data = dt_messages, .f_get_type = NULL },
};

static struct {
    gchar *pattern;
    int type;
    GPatternSpec *pspec; /* compiled later */
} prop_types[] = {
    { "name", DTP_STR, NULL },
    { "compatible", DTP_STR, NULL },
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
    { "operating-points-v2", DTP_PH_REF_OPP2, NULL },
    { "opp-hz", DTP_UINT64, NULL },
    { "opp-microvolt", DTP_UINT, NULL },
    { "opp-microvolt-*", DTP_UINT, NULL }, /* opp-microvolt-<named> */
    { "opp-microamp", DTP_UINT, NULL },
    { "clock-latency-ns", DTP_UINT, NULL },
};

/*
static struct {
    char *name; uint32_t v;
} default_values[] = {
    { "#address-cells", 2 },
    { "#size-cells", 1 },
    { "#dma-cells", 1 },
    { NULL, 0 },
};
*/

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

static gchar *dt_messages(const gchar *path) {
    PARAM_NOT_UNUSED(path);
    return g_strdup(dtr_log ? dtr_log : "");
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

    if (obj->is_dir)
        return DT_NODE;

    if (obj->data.len == 0)
        return DTP_EMPTY;

    /* special #(.*)-cells names are UINT */
    if (*obj->name == '#') {
        dash = strrchr(obj->name, '-');
        if (dash != NULL) {
            if (strcmp(dash, "-cells") == 0)
                return DTP_UINT;
        }
    }

    /* /aliases/ * and /__symbols__/ * are always strings */
    if (strncmp(obj->path, "/aliases/", strlen("/aliases/") ) == 0)
        return DTP_STR;
    if (strncmp(obj->path, "/__symbols__/", strlen("/__symbols__/") ) == 0)
        return DTP_STR;

    /* /__overrides__/ * */
    if (strncmp(obj->path, "/__overrides__/", strlen("/__overrides__/") ) == 0)
        if (strcmp(obj->name, "name") != 0)
            return DTP_OVR;

    /* lookup known type */
    int nt = dtr_guess_type_by_name(obj->name);
    if (nt) return nt;

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

static guint dtr_flags(sysobj *obj) {
    if (obj)
        return obj->cls->flags;
    return CLS_DT_FLAGS;
}

static gchar *dtr_format(sysobj *obj, int fmt_opts) {
    int type = dtr_guess_type(obj);
    switch(type) {
        case DT_NODE:
            return g_strdup("{node}");
        case DTP_EMPTY:
            if (fmt_opts & FMT_OPT_NULL_IF_EMPTY)
                return NULL;
            return g_strdup("{empty}");
        case DTP_STR:
            if (fmt_opts & FMT_OPT_NULL_IF_EMPTY) {
                if (obj->data.len == 1 && *obj->data.str == 0)
                    return NULL;
            }
            return dtr_list_str0(obj->data.str, obj->data.len);
        case DTP_UINT:
            return dtr_list_uint32(obj->data.uint32, obj->data.len / 4);
        case DTP_UINT64:
            return dtr_list_uint64(obj->data.uint64, obj->data.len / 8);
        case DTP_REG:
        case DTP_INTRUPT:
        case DTP_INTRUPT_EX:
        case DTP_CLOCKS:
        case DTP_GPIOS:
        case DTP_DMAS:
        case DTP_PH:
        case DTP_HEX:
            if (obj->data.len % 4)
                return dtr_list_byte((uint8_t*)obj->data.any, obj->data.len);
            else
                return dtr_list_hex(obj->data.any, obj->data.len / 4);
        case DTP_UNK:
        default:
            if (obj->data.len > 64) /* maybe should use #define at the top */
                return g_strdup_printf("{data} (%lu bytes)", obj->data.len);
            else
                return dtr_list_byte((uint8_t*)obj->data.any, obj->data.len);
    }
    return simple_format(obj, fmt_opts);
}

void class_dt_cleanup() {
    int i = 0;
    for (i = 0; i < (int)G_N_ELEMENTS(prop_types); i++) {
        if (prop_types[i].pspec)
            g_pattern_spec_free(prop_types[i].pspec);
    }
    g_free(dtr_log);
}

void class_dt() {
    int i = 0;

    dtr_log = g_strdup("");

    /* add virtual sysobj */
    for (i = 0; i < (int)G_N_ELEMENTS(vol); i++) {
        sysobj_virt_add(&vol[i]);
    }

    /* add classes */
    for (i = 0; i < (int)G_N_ELEMENTS(cls_dtr); i++) {
        class_add(&cls_dtr[i]);
    }

    /* compile type table patterns */
    for (i = 0; i < (int)G_N_ELEMENTS(prop_types); i++) {
        if ( strchr(prop_types[i].pattern,'*')
            || strchr(prop_types[i].pattern,'?') )
            prop_types[i].pspec = g_pattern_spec_new(prop_types[i].pattern);
    }

    if (!sysobj_exists_from_fn(DTROOT, NULL)) {
        dtr_msg("devicetree not found at %s", DTROOT);
    }
}

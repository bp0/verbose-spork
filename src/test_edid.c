
#include "sysobj.h"
#include "sysobj_foreach.h"
#include "sysobj_extras.h"
#include "util_sysobj.h"
#include "util_edid.h"
#include <unistd.h> /* for isatty() */

struct {
    char *tag;
    int len;
    char *edid_data;
} test_data[] = {
{ "JVC", 256,
"		00ffffffffffff002ac301001f000000\n"
"		1615010380522e780aa13ba3554a9b25\n"
"		10474aa5cf0001010101010101010101\n"
"		010101010101023a801871382d40582c\n"
"		450033cd3100001e000000fd00324d1f\n"
"		460f000a202020202020000000ff0031\n"
"		304142303131363030303031000000fc\n"
"		004a4c4333374243333030300a20013b\n"
"		020324714b010304059020151213141f\n"
"		29090705155750000000830100006503\n"
"		0c001000023a801871382d40582c5500\n"
"		33cd3100001e011d007251d01e206e28\n"
"		550033cd3100001e0000000000000000\n"
"		00000000000000000000000000000000\n"
"		00000000000000000000000000000000\n"
"		000000000000000000000000000000b4\n" },
{ "IBM", 128,
"		00ffffffffffff00244d8e1900000000\n"
"		0a05010108281eb4c800b2a057499b26\n"
"		10484fa4cf7c314aa940a94aa94f8180\n"
"		010101010101100bd0b4205e6310126c\n"
"		6208fab80000001a000000ff00333039\n"
"		41424330303032350a20000000fe0054\n"
"		48495320495320410a202020000000fe\n"
"		00544553542c2054484520454e44008f\n" },
{ "Dell", 128,
"		00ffffffffffff0010ac3b404d384730\n"
"		0e130103682f1e782eee95a3544c9926\n"
"		0f5054a54b00714f8180b30001010101\n"
"		01010101010121399030621a274068b0\n"
"		3600d9281100001c000000ff00483036\n"
"		39483934333047384d0a000000fc0044\n"
"		454c4c20323230385746500a000000fd\n"
"		00384b1e5310000a2020202020200095\n" },
{ "HP", 128,
"		00ffffffffffff0022f0a82601010101\n"
"		2b110103682f1e78eeb535a5564a9a25\n"
"		105054a56b90710081409500a900b300\n"
"		01010101010121399030621a274068b0\n"
"		3600d9281100001c000000fd00304c18\n"
"		5210000a202020202020000000fc0048\n"
"		502077323230370a20202020000000ff\n"
"		00434e44373433334b50440a202000d8\n" },
{ "LGD Haruka", 128,
"		00ffffffffffff0030e42f0500000000\n"
"		001a01049522137803a1c59459578f27\n"
"		20505400000001010101010101010101\n"
"		0101010101012e3680a070381f403020\n"
"		350058c210000019222480a070381f40\n"
"		3020350058c210000019000000fd0028\n"
"		3c43430e010a20202020202000000002\n"
"		000c47ff0a3c6e1c151f6e0000000018\n" },
{ "LGD Setsuna", 128,
"		00ffffffffffff0030e4f70200000000\n"
"		00140103802615780a88a59d5f579c26\n"
"		1c505400000001010101010101010101\n"
"		0101010101012f2640a060841a303020\n"
"		35007ed7100000190000000000000000\n"
"		00000000000000000000000000fe004c\n"
"		4720446973706c61790a2020000000fe\n"
"		004c503137335744312d544c4e31000e\n" },
{ "LGD Pumbler", 128,
"		00ffffffffffff0030e401d800000000\n"
"		00120103802213780ade05a35a519226\n"
"		1a505400000001010101010101010101\n"
"		0101010101013e1c56a0500016303020\n"
"		350058c2100000190000000000000000\n"
"		00000000000000000000000000fe004c\n"
"		4720446973706c61790a2020000000fe\n"
"		004c503135365748312d544c4331004f\n" },
};

static int fmt_opts_complete = FMT_OPT_NO_JUNK | FMT_OPT_COMPLETE;
static int fmt_opts_list = FMT_OPT_NO_JUNK | FMT_OPT_LIST_ITEM;

gboolean examine(sysobj *s, gpointer user_data, gconstpointer stats) {
    if (SEQ(s->name, "edid")) {
        sysobj_classify(s);
        sysobj_read(s, FALSE);
        gchar *nice_li = sysobj_format(s, fmt_opts_list);
        gchar *nice = sysobj_format(s, fmt_opts_complete);
        printf("\n-- %s --\n-[ %s ]-\n%s\n", s->path, nice_li, nice);

        edid *e = edid_new(s->data.any, s->data.len);
        if (e) {
            char *hexver = edid_dump_hex(e, 2, 1);
            printf("%s\n", hexver);
            g_free(hexver);
            edid_free(e);
        }

        g_free(nice_li);
        g_free(nice);

    }
    return SYSOBJ_FOREACH_CONTINUE;
}

int main(int argc, char **argv) {
    const gchar *altroot = NULL;

    if (argc >= 2) {
        altroot = argv[1];
    }

    if (isatty(fileno(stdout)) ) {
        fmt_opts_list |= FMT_OPT_ATERM;
        fmt_opts_complete |= FMT_OPT_ATERM;
    }

    sysobj_init(altroot);

    sysobj_foreach_from("/sys/devices", NULL, (f_sysobj_foreach)examine, NULL, SO_FOREACH_NORMAL);

    for(int i = 0; i < (int)G_N_ELEMENTS(test_data); i++) {
        edid_basic id;
        edid *e = edid_new_from_hex(test_data[i].edid_data);
        char *out = edid_dump2(e);
        printf("\n-- test_data[%d] %s --\n%s\n", i, test_data[i].tag, out);
        g_free(out);
        edid_free(e);
    }

    sysobj_cleanup();
    return 0;
}

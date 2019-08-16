
#include "sysobj.h"
#include "sysobj_foreach.h"
#include "sysobj_extras.h"
#include "util_sysobj.h"
#include "util_edid.h"
#include <unistd.h> /* for isatty() */

char xrr_test_data[] =
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
"		000000000000000000000000000000b4\n";

static int fmt_opts_complete = FMT_OPT_NO_JUNK | FMT_OPT_COMPLETE;
static int fmt_opts_list = FMT_OPT_NO_JUNK | FMT_OPT_LIST_ITEM;

gboolean examine(sysobj *s, gpointer user_data, gconstpointer stats) {
    if (SEQ(s->name, "edid")) {
        sysobj_classify(s);
        sysobj_read(s, FALSE);
        gchar *nice_li = sysobj_format(s, fmt_opts_list);
        gchar *nice = sysobj_format(s, fmt_opts_complete);
        printf("\n-- %s --\n-[ %s ]-\n%s\n", s->path, nice_li, nice);
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

    struct edid id = {};
    edid_fill_xrandr(&id, xrr_test_data);
    char *out = edid_dump(&id);
    printf("\n-- xrr_test_data --\n%s\n", out);

    sysobj_cleanup();
    return 0;
}

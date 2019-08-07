
#include "sysobj.h"
#include "sysobj_foreach.h"

static int fmt_opts = FMT_OPT_NO_JUNK | FMT_OPT_LIST_ITEM;

gboolean examine(sysobj *s, gpointer user_data, gconstpointer stats) {
    if (SEQ(s->name, "compatible")) {
        sysobj_classify(s);
        sysobj_read(s, FALSE);
        gchar *nice = sysobj_format(s, fmt_opts);
        printf("%s: %s\n", s->path, nice);
        g_free(nice);
    }
    return SYSOBJ_FOREACH_CONTINUE;
}

int main(int argc, char **argv) {
    const gchar *altroot = NULL;

    if (argc >= 2) {
        altroot = argv[1];
    }

    if (isatty(fileno(stdout)) )
        fmt_opts |= FMT_OPT_ATERM;

    sysobj_init(altroot);

    sysobj *bo = sysobj_new_from_fn("/sys/firmware/devicetree/base", NULL);
    if (!bo->exists)
        fprintf(stderr, "No device tree found at /sys/firmware/devicetree/base\n");
    else
        sysobj_foreach_from("/sys/firmware/devicetree/base", NULL, (f_sysobj_foreach)examine, NULL, SO_FOREACH_NORMAL);
    sysobj_free(bo);
    sysobj_cleanup();
    return 0;
}

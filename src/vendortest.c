
#include "sysobj.h"
#include "format_funcs.h"
#include "spd-vendors.c"

char *Xstrstr_word(const char *haystack, const char *needle) {
    if (!haystack || !needle)
        return NULL;

    char *c;
    const char *p = haystack;
    size_t l = strlen(needle);
    while(c = strstr(p, needle)) {
        const char *before = (c == haystack) ? NULL : c-1;
        const char *after = c + l;
        int ok = 1;
        printf("consider: %s -> ['%c','%c']\n", c, before ? *before : '^', *after);
        if (isalnum(*after)) ok = 0;
        if (before && isalnum(*before)) ok = 0;
        if (ok) return c;
        p++;
    }
    return NULL;
}

int main(int argc, char **argv) {
    sysobj_init(NULL);

    int b,i;

    for(b = 0; b < VENDORS_BANKS; b++) {
        for(i = 0; i < VENDORS_ITEMS; i++) {
            const gchar *vendor_str = JEDEC_MFG_STR(b,i);
            if (!vendor_str) continue;
            //if (!strstr(vendor_str, "former")) continue;
            gchar *mstr = vendor_match_tag(vendor_str, FMT_OPT_ATERM);
            if (mstr)
                printf("-- [%02d,%03d] %s  ===  %s\n", b,i, vendor_str, mstr );
            g_free(mstr);
        }
    }

    sysobj_cleanup();
    return 0;
}

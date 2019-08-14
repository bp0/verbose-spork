
#include "sysobj.h"
#include "format_funcs.h"
#include "spd-vendors.c"

const gchar *linux_vendors[] = {
    "Debian", "Ubuntu", "Raspbian", "Canonical", "deepin",
    "Red Hat", "Fedora",
    "Arch",
    "Gentoo",
    "Slackware", "Zenwalk",
    "Void",
    "KDE", "GNOME", "X.org", "MATE",
    "Linux",
};

const struct { char *test_str, *good, *bad; }
special_case_tests[] = {
    { "Harmony", .bad="arm" },
    { "Transcend", .good="Transcend Information" },
    { "Transcend Technology Co. Ltd.", .bad="Transcend Information" },
    { "ST3160023A", .good="Seagate" },
    { "STEC", .bad="Seagate" },
    { "ST Whatever", .bad="Seagate" },
    { "Digital", .good="Digital Equipment Corporation" },
    { "American Computer & Digital Components Inc", .bad="Digital Equipment Corporation" },
};

int main(int argc, char **argv) {
    vendor_die_on_error = TRUE;
    sysobj_init(NULL);

    unsigned int b,i;

    int spdv_total = 0, spdv_hit = 0;
    for(b = 0; b < VENDORS_BANKS; b++) {
        for(i = 0; i < VENDORS_ITEMS; i++) {
            const gchar *vendor_str = JEDEC_MFG_STR(b,i);
            if (!vendor_str) continue;
            spdv_total++;
            vendor_list vl = vendors_match(vendor_str, NULL);
            vl = vendor_list_remove_duplicates_deep(vl);
            int vc = g_slist_length(vl);
            gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
            if (mstr) {
                spdv_hit++;
                printf("-- jedec [%02d,%03d] %s  === (%d) %s \n", b,i, vendor_str, vc, mstr );
            }
            g_free(mstr);
        }
    }

    int osv_total = 0, osv_hit = 0;
    for(i = 0; i < G_N_ELEMENTS(linux_vendors); i++) {
            osv_total++;
            vendor_list vl = vendors_match(linux_vendors[i], NULL);
            vl = vendor_list_remove_duplicates_deep(vl);
            int vc = g_slist_length(vl);
            gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
            if (mstr) {
                osv_hit++;
                printf("-- os_release: %s  === (%d) %s \n", linux_vendors[i], vc, mstr );
            }
            g_free(mstr);
    }

    gchar *p = NULL, *next_nl = NULL;

    gchar *pci_ids = NULL;
    int pci_total = 0, pci_hit = 0;
    if (g_file_get_contents("pci.ids", &pci_ids, NULL, NULL)) {
        gchar *p = pci_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (strlen(p) > 5
                && isxdigit(p[0])
                && isxdigit(p[1])
                && isxdigit(p[2])
                && isxdigit(p[3])
                && isspace(p[4]) ) {
                    pci_total++;
                    vendor_list vl = vendors_match(p+5, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        pci_hit++;
                        p[4] = 0;
                        printf("-- pci [%s] %s  === (%d) %s \n", p, p+5, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(pci_ids);
    }

    gchar *usb_ids = NULL;
    int usb_total = 0, usb_hit = 0;
    if (g_file_get_contents("usb.ids", &usb_ids, NULL, NULL)) {
        gchar *p = usb_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (strlen(p) > 5
                && isxdigit(p[0])
                && isxdigit(p[1])
                && isxdigit(p[2])
                && isxdigit(p[3])
                && isspace(p[4]) ) {
                    usb_total++;
                    vendor_list vl = vendors_match(p+5, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        usb_hit++;
                        p[4] = 0;
                        printf("-- usb [%s] %s  === (%d) %s \n", p, p+5, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(usb_ids);
    }

    gchar *edid_ids = NULL;
    int edid_total = 0, edid_hit = 0;
    if (g_file_get_contents("edid.ids", &edid_ids, NULL, NULL)) {
        gchar *p = edid_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (strlen(p) > 5
                && isalnum(p[0])
                && isalnum(p[1])
                && isalnum(p[2])
                && isspace(p[3]) ) {
                    edid_total++;
                    vendor_list vl = vendors_match(p+4, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        edid_hit++;
                        p[3] = 0;
                        printf("-- edid [%s] %s  === (%d) %s \n", p, p+4, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(edid_ids);
    }

    gchar *sdio_ids = NULL;
    int sdio_total = 0, sdio_hit = 0;
    if (g_file_get_contents("sdio.ids", &sdio_ids, NULL, NULL)) {
        gchar *p = sdio_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (strlen(p) > 5
                && isxdigit(p[0])
                && isxdigit(p[1])
                && isxdigit(p[2])
                && isxdigit(p[3])
                && isspace(p[4]) ) {
                    sdio_total++;
                    vendor_list vl = vendors_match(p+5, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        sdio_hit++;
                        p[4] = 0;
                        printf("-- sdio [%s] %s  === (%d) %s \n", p, p+5, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(sdio_ids);
    }

    gchar *sdcard_ids = NULL;
    int sdcard_total = 0, sdcard_hit = 0;
    if (g_file_get_contents("sdcard.ids", &sdcard_ids, NULL, NULL)) {
        gchar *p = sdcard_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (g_str_has_prefix(p, "MANFID")
                || g_str_has_prefix(p, "OEMID") ) {
                    gchar *s = strchr(p+8, '#');
                    if (s) *s = 0;
                    s = strchr(p+8, ' ');
                    if (s) *s = 0;
                    sdcard_total++;
                    vendor_list vl = vendors_match(s+1, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        sdcard_hit++;
                        printf("-- sdcard [%s] %s  === (%d) %s \n", p, s+1, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(sdcard_ids);
    }

    gchar *dt_ids = NULL;
    int dt_total = 0, dt_hit = 0;
    if (g_file_get_contents("dt.ids", &dt_ids, NULL, NULL)) {
        gchar *p = dt_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (g_str_has_prefix(p, "vendor ")) {
                    gchar *s = strchr(p, '#');
                    if (s) *s = 0;
                    s = strchr(p, ' ');
                    if (s) *s = 0;
                    dt_total++;
                    vendor_list vl = vendors_match(s+1, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        dt_hit++;
                        printf("-- dt [%s] %s  === (%d) %s \n", p, s+1, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(dt_ids);
    }

    gchar *arm_ids = NULL;
    int arm_total = 0, arm_hit = 0;
    if (g_file_get_contents("arm.ids", &arm_ids, NULL, NULL)) {
        gchar *p = arm_ids;
        while(next_nl = strchr(p, '\n')) {
            *next_nl = 0;
            if (strlen(p) > 5
                && isxdigit(p[0])
                && isxdigit(p[1])
                && isspace(p[2]) ) {
                    arm_total++;
                    vendor_list vl = vendors_match(p+3, NULL);
                    vl = vendor_list_remove_duplicates_deep(vl);
                    int vc = g_slist_length(vl);
                    gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
                    if (mstr) {
                        arm_hit++;
                        p[2] = 0;
                        printf("-- arm [%s] %s  === (%d) %s \n", p, p+3, vc, mstr );
                    }
                    g_free(mstr);

            }
            p = next_nl + 1;
        }
        g_free(arm_ids);
    }

    int sc_total = 0, sc_hit = 0, sc_success = 0, sc_fail = 0;
    for(i = 0; i < G_N_ELEMENTS(special_case_tests); i++) {
            sc_total++;
            vendor_list vl = vendors_match(special_case_tests[i].test_str, NULL);
            vendor_list vlg = vendors_match(special_case_tests[i].good, NULL);
            vendor_list vlb = vendors_match(special_case_tests[i].bad, NULL);
            vl = vendor_list_remove_duplicates_deep(vl);
            vlg = vendor_list_remove_duplicates_deep(vlg);
            const Vendor *v = vl ? vl->data : NULL;
            const Vendor *vg = vlg ? vlg->data : NULL;
            const Vendor *vb = vlb ? vlb->data : NULL;
            int vc = g_slist_length(vl);
            int vcg = g_slist_length(vlg);
            int vcb = g_slist_length(vlb);
            gchar *mstr = vendor_list_ribbon(vl, FMT_OPT_ATERM);
            gchar *mstrg = vendor_list_ribbon(vlg, FMT_OPT_ATERM);
            gchar *mstrb = vendor_list_ribbon(vlb, FMT_OPT_ATERM);
            if (mstr) sc_hit++;
            printf("-- test: %s \n    --- result: (%d) %s \n", special_case_tests[i].test_str, vc, mstr );
            gboolean mg = TRUE;
            if (special_case_tests[i].good) {
                mg = vendor_cmp_deep(v, vg) == 0 ? 1 : 0;
                printf("    --- good: (%d) %s   ---> %s \n", vcg, mstrg, mg ? "[HIT]" : "[MISS]");
            }
            gboolean mb = FALSE;
            if (special_case_tests[i].bad) {
                mb = vendor_cmp_deep(v, vb) == 0 ? 1 : 0;
                printf("    --- bad: (%d) %s   ---> %s \n", vcb, mstrb, mb ? "[HIT]" : "[MISS]");
            }
            gboolean pass = TRUE;
            pass = pass && mg;
            pass = pass && !mb;
            if (pass) {
                sc_success++;
                printf("    ----- %s.\n", special_case_tests[i].good ? "OK" : "OK  (probably)");
            } else {
                sc_fail++;
                printf("    ***** FAIL!\n");
            }
            g_free(mstr);
            g_free(mstrg);
            g_free(mstrb);
    }

    printf("\n" "Coverage:\n"
           "Linux/os_release: %d / %d\n"
           "JEDEC Mfgr: %d / %d\n"
           "PCI Vendor: %d / %d\n"
           "USB Vendor: %d / %d\n"
           "EDID Vendor: %d / %d\n"
           "Device Tree Vendor: %d / %d\n"
           "ARM Mfgr: %d / %d\n"
           "SD Card Vendor/Mfgr: %d / %d\n"
           "SDIO Vendor: %d / %d\n"
           "Special Test: %d tests; %d success; %d failure\n",
           osv_hit, osv_total,
           spdv_hit, spdv_total,
           pci_hit, pci_total,
           usb_hit, usb_total,
           edid_hit, edid_total,
           dt_hit, dt_total,
           arm_hit, arm_total,
           sdcard_hit, sdcard_total,
           sdio_hit, sdio_total,
           sc_total, sc_success, sc_fail
           );

    sysobj_cleanup();
    return 0;
}

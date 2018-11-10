/*
 *    rpiz - https://github.com/bp0/rpiz
 *    Copyright (C) 2017  Burt P. <pburt0@gmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

static char unk[] = "(Unknown)";

/* information table from: http://elinux.org/RPi_HardwareHistory */
static struct {
    uint32_t rev; char *rstr, *intro, *model, *pcb, *mem, *mfg, *soc;
} rpi_boardinfo[] = {
/*  Revision            Introduction  Model Name             PCB rev.  Memory(spec)   Manufacturer  SOC(spec) *
 *                                    Raspberry Pi %s                                                         */
  { 0x0,  unk,          unk,          unk,                   unk,      unk,        unk,             NULL },
  { 0x1,  "Beta",       "Q1 2012",    "B (Beta)",            unk,      "256MB",    "(Beta board)",  NULL },
  { 0x2,  "0002",       "Q1 2012",    "B",                   "1.0",    "256MB",    unk,             "BCM2835" },
  { 0x3,  "0003",       "Q3 2012",    "B (ECN0001)",         "1.0",    "256MB",    "(Fuses mod and D14 removed)",   NULL },
  { 0x4,  "0004",       "Q3 2012",    "B",                   "2.0",    "256MB",    "Sony",          NULL },
  { 0x5,  "0005",       "Q4 2012",    "B",                   "2.0",    "256MB",    "Qisda",         NULL },
  { 0x6,  "0006",       "Q4 2012",    "B",                   "2.0",    "256MB",    "Egoman",        NULL },
  { 0x7,  "0007",       "Q1 2013",    "A",                   "2.0",    "256MB",    "Egoman",        NULL },
  { 0x8,  "0008",       "Q1 2013",    "A",                   "2.0",    "256MB",    "Sony",          NULL },
  { 0x9,  "0009",       "Q1 2013",    "A",                   "2.0",    "256MB",    "Qisda",         NULL },
  { 0xd,  "000d",       "Q4 2012",    "B",                   "2.0",    "512MB",    "Egoman",        NULL },
  { 0xe,  "000e",       "Q4 2012",    "B",                   "2.0",    "512MB",    "Sony",          NULL },
  { 0xf,  "000f",       "Q4 2012",    "B",                   "2.0",    "512MB",    "Qisda",         NULL },
  { 0x10, "0010",       "Q3 2014",    "B+",                  "1.0",    "512MB",    "Sony",          NULL },
  { 0x11, "0011",       "Q2 2014",    "Compute Module 1",    "1.0",    "512MB",    "Sony",          NULL },
  { 0x12, "0012",       "Q4 2014",    "A+",                  "1.1",    "256MB",    "Sony",          NULL },
  { 0x13, "0013",       "Q1 2015",    "B+",                  "1.2",    "512MB",    unk,             NULL },
  { 0x14, "0014",       "Q2 2014",    "Compute Module 1",    "1.0",    "512MB",    "Embest",        NULL },
  { 0x15, "0015",       unk,          "A+",                  "1.1",    "256MB/512MB",    "Embest",      NULL  },
  { 0xa01040, "a01040", unk,          "2 Model B",           "1.0",    "1GB",      "Sony",          "BCM2836" },
  { 0xa01041, "a01041", "Q1 2015",    "2 Model B",           "1.1",    "1GB",      "Sony",          "BCM2836" },
  { 0xa21041, "a21041", "Q1 2015",    "2 Model B",           "1.1",    "1GB",      "Embest",        "BCM2836" },
  { 0xa22042, "a22042", "Q3 2016",    "2 Model B",           "1.2",    "1GB",      "Embest",        "BCM2837" },  /* (with BCM2837) */
  { 0x900021, "900021", "Q3 2016",    "A+",                  "1.1",    "512MB",    "Sony",          NULL },
  { 0x900032, "900032", "Q2 2016?",    "B+",                 "1.2",    "512MB",    "Sony",          NULL },
  { 0x900092, "900092", "Q4 2015",    "Zero",                "1.2",    "512MB",    "Sony",          NULL },
  { 0x900093, "900093", "Q2 2016",    "Zero",                "1.3",    "512MB",    "Sony",          NULL },
  { 0x920093, "920093", "Q4 2016?",   "Zero",                "1.3",    "512MB",    "Embest",        NULL },
  { 0x9000c1, "9000c1", "Q1 2017",    "Zero W",              "1.1",    "512MB",    "Sony",          NULL },
  { 0xa02082, "a02082", "Q1 2016",    "3 Model B",           "1.2",    "1GB",      "Sony",          "BCM2837" },
  { 0xa020a0, "a020a0", "Q1 2017",    "Compute Module 3 or CM3 Lite",  "1.0",    "1GB",    "Sony",          NULL },
  { 0xa22082, "a22082", "Q1 2016",    "3 Model B",           "1.2",    "1GB",      "Embest",        "BCM2837" },
  { 0xa32082, "a32082", "Q4 2016",    "3 Model B",           "1.2",    "1GB",      "Sony Japan",    NULL  },
  { 0xa020d3, "a020d3", "Q4 2018",    "3 Model B+",          "1.3",    "1GB",      "Sony",          "BCM2837" },
};

/* return number of chars to skip */
static int rpi_ov_check(const char *r_code) {
    /* sources differ. prefix is either 1000... or just 1... */
    //if (strncmp(r, "1000", 4) == 0)
    //    return 4;
    if (strncmp(r_code, "1", 1) == 0)
        return 1;
    return 0;
}

static int rpi_code_match(const char* code0, const char* code1) {
    int c0, c1;
    if (code0 == NULL || code1 == NULL) return 0;
    c0 = strtol(code0, NULL, 16);
    c1 = strtol(code1, NULL, 16);
    if (c0 && c1)
        return (c0 == c1) ? 1 : 0;
    else
        return (strcmp(code0, code1) == 0) ? 1 : 0;
}

static int rpi_find_board(const char *r_code) {
    int i = 0;
    char *r = (char*)r_code;
    if (r_code == NULL)
        return 0;
    /* ignore the overvolt prefix */
    r += rpi_ov_check(r_code);
    while (rpi_boardinfo[i].rstr != NULL) {
        if (rpi_code_match(r, rpi_boardinfo[i].rstr))
            return i;

        i++;
    }
    return 0;
}

/*
static gchar *mk_r_code(uint32_t r) {
    return g_strdup_printf("%04x", r);
}
*/

#define DTROOT "/sys/firmware/devicetree/base"

static gchar *rpi_board_details(void) {
    int i = 0;
    gchar *ret = NULL;
    gchar *soc = NULL;
    gchar *serial = NULL;
    gchar *revision = NULL;
    int ov = 0;
    FILE *cpuinfo;
    gchar buffer[128];

    /* TODO: ask nicely? */
    /* DTROOT, "system/linux,revision" */
    /* DTROOT, "system/linux,serial" */

    /* try cpuinfo */
    if (!revision) {
        sysobj *obj = sysobj_new_from_fn("/proc/cpuinfo", NULL);
        if (obj && obj->exists) {
            sysobj_read_data(obj);
            revision = util_find_line_value(obj->data.str, "Revision", ":");
            serial = util_find_line_value(obj->data.str, "Serial", ":");
            soc = util_find_line_value(obj->data.str, "Hardware", ":");
        }
        sysobj_free(obj);
    }

    if (revision == NULL || soc == NULL)
        goto rpi_board_details_done;

    ov = rpi_ov_check(revision);
    i = rpi_find_board(revision);
    ret = g_strdup_printf("[raspberry_pi]\n"
                "board_name=%s %s\n"
                "pcb_revision=%s\n"
                "introduction=%s\n"
                "manufacturer=%s\n"
                "r_code=%s\n"
                "spec_soc=%s\n"
                "spec_mem=%s\n"
                "serial=%s\n"
                "overvolt=%s\n",
                "Raspberry Pi", rpi_boardinfo[i].model,
                rpi_boardinfo[i].pcb,
                rpi_boardinfo[i].intro,
                rpi_boardinfo[i].mfg,
                (rpi_boardinfo[i].rstr != unk) ? rpi_boardinfo[i].rstr : revision,
                rpi_boardinfo[i].soc,
                rpi_boardinfo[i].mem,
                serial ? serial : "",
                (ov) ? "1" : "0" );

rpi_board_details_done:
    g_free(soc);
    g_free(revision);
    g_free(serial);
    return ret;
}

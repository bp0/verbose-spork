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

#include "sysobj.h"

#define SEQ(s1, s2) (g_strcmp0(s1, s2) == 0)

#define BULLET "\u2022"
#define REFLINK(URI) "<a href=\"" URI "\">" URI "</a>"
const gchar hwmon_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface")
    "\n";

static gboolean hwmon_verify(sysobj *obj);
static gchar *hwmon_format(sysobj *obj, int fmt_opts);

static gboolean hwmon_attr_verify(sysobj *obj);
static gchar *hwmon_attr_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_hwmon[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "hwmon", .pattern = "/sys/devices/*/hwmon*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = hwmon_reference_markup_text,
    .f_verify = hwmon_verify, .f_format = hwmon_format },
  { SYSOBJ_CLASS_DEF
    .tag = "hwmon:attr", .pattern = "/sys/devices/*/hwmon*/*", .flags = OF_GLOB_PATTERN | OF_CONST,
    .s_halp = hwmon_reference_markup_text, .s_update_interval = 0.2,
    .f_verify = hwmon_attr_verify, .f_format = hwmon_attr_format },
};

static gboolean hwmon_verify(sysobj *obj) {
    return verify_lblnum(obj, "hwmon");
}

static gchar *hwmon_format(sysobj *obj, int fmt_opts) {
    gchar *name = sysobj_raw_from_fn(obj->path, "name");
    if (name) {
        g_strstrip(name);
        return name;
    }
    return simple_format(obj, fmt_opts);
}

static const char *sensor_types[] = {
    "in", "temp", "cpu", "fan", "pwm", "temp", "curr", "power", "energy", "humidity", "intrusion"
};

/* temp4_crit -> "temp", 4, "crit" */
static gboolean hwmon_attr_decode_name(const gchar *name, gchar **type, int *index, gchar **attrib) {
    if (!name) return FALSE;

    /* split at first _ */
    gchar *t = g_strdup(name);
    gchar *undy = strchr(t, '_');
    gchar *a = undy ? undy + 1 : NULL;
    if (undy) *undy = 0;

    /* find index */
    int tn = -1;
    gchar *n = t;
    while(*n && !isdigit(*n)) n++;
    if (*n) {
        tn = atoi(n);
        *n = 0;
    }

    /* verify the name */
    int ok = 0;
    for(int i = 0; i<(int)G_N_ELEMENTS(sensor_types); i++) {
        if (SEQ(sensor_types[i], t) )
            ok = 1;
    }
    if (!ok) {
        g_free(t);
        return FALSE;
    }

    /* lgtm */
    if (type) {
        if (*type) g_free(*type);
        *type = t;
    }
    if (index)
        *index = tn;
    if (attrib) {
        if (*attrib) g_free(*attrib);
            *attrib = g_strdup(a);
    }
    return TRUE;
}

static gboolean hwmon_attr_verify(sysobj *obj) {
    if ( verify_lblnum_child(obj, "hwmon") ) {
        if (hwmon_attr_decode_name(obj->name, NULL, NULL, NULL) ) {
            return TRUE;
        }
    }
    return FALSE;
}

static gchar *fmt_beep_channel(sysobj *obj, int fmt_opts) {
    static const char *beep[] = {
        N_("no beep (channel)"),
        N_("beep (channel)"),
        NULL
    };
    return sysobj_format_table(obj, (gchar**)&beep, (int)G_N_ELEMENTS(beep), 10, fmt_opts);
}

static gchar *fmt_alarm_channel(sysobj *obj, int fmt_opts) {
    static const char *alarm[] = {
        N_("no alarm (channel)"),
        N_("alarm (channel)"),
        NULL
    };
    return sysobj_format_table(obj, (gchar**)&alarm, (int)G_N_ELEMENTS(alarm), 10, fmt_opts);
}

static gchar *fmt_fault(sysobj *obj, int fmt_opts) {
    static const char *fault[] = {
        N_("no fault occurred"),
        N_("fault condition"),
        NULL
    };
    return sysobj_format_table(obj, (gchar**)&fault, (int)G_N_ELEMENTS(fault), 10, fmt_opts);
}

static gchar *fmt_enable(sysobj *obj, int fmt_opts) {
    static const char *disable_enable[] = {
        N_("disable"),
        N_("enable"),
        NULL
    };
    return sysobj_format_table(obj, (gchar**)&disable_enable, (int)G_N_ELEMENTS(disable_enable), 10, fmt_opts);
}

static gchar *fmt_rpm(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double rpm = strtol(trim, NULL, 10);
    g_free(trim);
    return g_strdup_printf("%.1lf %s", rpm, _("RPM") );
}

static gchar *fmt_pwm_level(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double pwm = strtol(obj->data.str, NULL, 10);
    pwm = (pwm / 255) * 100;
    g_free(trim);
    return g_strdup_printf("%.1lf%s", pwm, _("%") );
}

static gchar *fmt_pwm_enable(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    gchar *m = N_("(Unknown)");
    int v = atoi(trim);
    g_free(trim);
    if (v == 0)
        m = N_("no fan speed control (i.e. fan at full speed)");
    if (v == 1)
        m = N_("manual fan speed control enabled (using pwm[1-*])");
    if (v > 1)
        m = N_("automatic fan speed control enabled");
    return g_strdup_printf("[%d] %s", v, _(m));
}

static gchar *fmt_pwm_mode(sysobj *obj, int fmt_opts) {
    static const char *pwm_mode[] = {
        N_("DC mode (direct current)"),
        N_("PWM mode (pulse-width modulation)"),
        NULL
    };
    return sysobj_format_table(obj, (gchar**)&pwm_mode, (int)G_N_ELEMENTS(pwm_mode), 10, fmt_opts);
}

static gchar *fmt_hz(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double rpm = strtol(trim, NULL, 10);
    g_free(trim);
    return g_strdup_printf("%.1lf %s", rpm, _("Hz") );
}

static gchar *fmt_millivolt(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double volt = strtol(obj->data.str, NULL, 10);
    volt /= 1000;
    g_free(trim);
    return g_strdup_printf("%.3lf %s", volt, _("V") );
}

static gchar *fmt_temp_type(sysobj *obj, int fmt_opts) {
    static const char *temp_types[] = {
        N_("(Unknown)"),
        N_("CPU embedded diode"),
        N_("3904 transistor"),
        N_("thermal diode"),
        N_("thermistor"),
        N_("AMD AMDSI"),
        N_("Intel PECI"),
        NULL
    };
    return sysobj_format_table(obj, (gchar**)&temp_types, (int)G_N_ELEMENTS(temp_types), 10, fmt_opts);
}

static gchar *fmt_millidegree_c(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double degc = strtol(obj->data.str, NULL, 10);
    degc /= 1000;
    g_free(trim);
    return g_strdup_printf("%.3lf %s", degc, _("\u00B0C") );
}

static gchar *fmt_milliampere(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double mA = strtol(obj->data.str, NULL, 10);
    g_free(trim);
    if (mA > 2000)
        return g_strdup_printf("%.3lf %s", (mA/1000), _("A") );
    else
        return g_strdup_printf("%.1lf %s", mA, _("mA") );
}

static gchar *fmt_microwatt(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double mW = strtol(obj->data.str, NULL, 10);
    mW /= 1000;
    g_free(trim);
    return g_strdup_printf("%.3lf %s", mW, _("mW") );
}

static gchar *fmt_milliseconds(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double ms = strtol(obj->data.str, NULL, 10);
    g_free(trim);
    return g_strdup_printf("%.1lf %s", ms, _("ms") );
}

static gchar *fmt_microjoule(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double mJ = strtol(obj->data.str, NULL, 10);
    mJ /= 1000;
    g_free(trim);
    return g_strdup_printf("%.3lf %s", mJ, _("mJ") );
}

static gchar *fmt_percent(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double perc = strtol(obj->data.str, NULL, 10);
    g_free(trim);
    return g_strdup_printf("%.1lf%s", perc, _("%") );
}

static gchar *fmt_millepercent(sysobj *obj, int fmt_opts) {
    gchar *trim = g_strstrip(g_strdup(obj->data.str));
    if (fmt_opts & FMT_OPT_NO_UNIT)
        return trim;
    double perc = strtol(obj->data.str, NULL, 10);
    perc /= 1000;
    g_free(trim);
    return g_strdup_printf("%.3lf%s", perc, _("%") );
}

static gchar *hwmon_attr_format(sysobj *obj, int fmt_opts) {
    gboolean temp_is_mv = FALSE; //TODO:
    gchar *type = NULL, *attrib = NULL;
    int i = -1;
    if (hwmon_attr_decode_name(obj->name, &type, &i, &attrib) ) {
        gchar *ret = NULL;
        if (SEQ(type, "in") ) {
            if (   SEQ(attrib, "min")
                || SEQ(attrib, "lcrit")
                || SEQ(attrib, "max")
                || SEQ(attrib, "crit")
                || SEQ(attrib, "input")
                || SEQ(attrib, "average")
                || SEQ(attrib, "lowest")
                || SEQ(attrib, "highest") )
                ret = fmt_millivolt(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "fault") )
                ret = fmt_fault(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "fan") ) {
            if (   SEQ(attrib, "min")
                || SEQ(attrib, "max")
                || SEQ(attrib, "input")
                || SEQ(attrib, "target") )
                ret = fmt_rpm(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "fault") )
                ret = fmt_fault(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "pwm") ) {
            if ( SEQ(attrib, NULL)
                || SEQ(attrib, "min")
                || SEQ(attrib, "max") )
                ret = fmt_pwm_level(obj, fmt_opts);
            else if ( SEQ(attrib, "freq") )
                ret = fmt_hz(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_pwm_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "fault") )
                ret = fmt_fault(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "temp") ) {
            if (   SEQ(attrib, "min")
                || SEQ(attrib, "max")
                || SEQ(attrib, "max_hyst")
                || SEQ(attrib, "min_hyst")
                || SEQ(attrib, "input")
                || SEQ(attrib, "crit")
                || SEQ(attrib, "crit_hyst")
                || SEQ(attrib, "emergency")
                || SEQ(attrib, "emergency_hyst")
                || SEQ(attrib, "lcrit")
                || SEQ(attrib, "lcrit_hyst")
                || SEQ(attrib, "offset")
                || SEQ(attrib, "lowest")
                || SEQ(attrib, "highest") )
                ret = temp_is_mv ? fmt_millivolt(obj, fmt_opts) : fmt_millidegree_c(obj, fmt_opts);
            else if (SEQ(attrib, "type") )
                ret = fmt_temp_type(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "fault") )
                ret = fmt_fault(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "curr") ) {
            if (   SEQ(attrib, "min")
                || SEQ(attrib, "max")
                || SEQ(attrib, "input")
                || SEQ(attrib, "crit")
                || SEQ(attrib, "lcrit")
                || SEQ(attrib, "average")
                || SEQ(attrib, "lowest")
                || SEQ(attrib, "highest") )
                ret = fmt_milliampere(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "fault") )
                ret = fmt_fault(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "power") ) {
            if (   SEQ(attrib, "average")
                || SEQ(attrib, "average_highest")
                || SEQ(attrib, "average_lowest")
                || SEQ(attrib, "average_max")
                || SEQ(attrib, "average_min")
                || SEQ(attrib, "max")
                || SEQ(attrib, "crit")
                || SEQ(attrib, "cap")
                || SEQ(attrib, "cap_max")
                || SEQ(attrib, "cap_min")
                || SEQ(attrib, "cap_hyst")
                || SEQ(attrib, "input")
                || SEQ(attrib, "input_lowest")
                || SEQ(attrib, "input_highest") )
                ret = fmt_microwatt(obj, fmt_opts);
            else if (   SEQ(attrib, "average_interval")
                || SEQ(attrib, "average_interval_max")
                || SEQ(attrib, "average_interval_min") )
                ret = fmt_milliseconds(obj, fmt_opts);
            else if ( SEQ(attrib, "accuracy") )
                ret = fmt_percent(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "fault") )
                ret = fmt_fault(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "energy") ) {
            if ( SEQ(attrib, "enable") )
                ret = fmt_microjoule(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "humidity") ) {
            if ( SEQ(attrib, "enable") )
                ret = fmt_millepercent(obj, fmt_opts);
            else if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        else if (SEQ(type, "intrusion") ) {
            if ( SEQ(attrib, "enable") )
                ret = fmt_enable(obj, fmt_opts);
            else if ( SEQ(attrib, "alarm") )
                ret = fmt_alarm_channel(obj, fmt_opts);
            else if ( SEQ(attrib, "beep") )
                ret = fmt_beep_channel(obj, fmt_opts);
        }
        g_free(type);
        g_free(attrib);
        if (ret)
            return ret;
    }
    return simple_format(obj, fmt_opts);
}

void class_hwmon() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_hwmon); i++)
        class_add(&cls_hwmon[i]);
}

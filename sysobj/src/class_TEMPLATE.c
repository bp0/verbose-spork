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

const gchar TEMPLATE_reference_markup_text[] =
    "Reference:\n"
    BULLET REFLINK("https://www.kernel.org/doc/Documentation/ABI/testing/TEMPLATE")
    "\n";

static gboolean TEMPLATE_verify(sysobj *obj);
static gchar *TEMPLATE_format(sysobj *obj, int fmt_opts);

static sysobj_class cls_TEMPLATE[] = {
  { SYSOBJ_CLASS_DEF
    .tag = "TEMPLATE", .pattern = "/sys/TEMPLATE", .flags = OF_CONST,
    .f_verify = TEMPLATE_verify, .f_format = TEMPLATE_format,
    .s_halp = TEMPLATE_reference_markup_text },
};

static gboolean TEMPLATE_verify(sysobj *obj) {
    return verify_true(obj);
}

static gchar *TEMPLATE_format(sysobj *obj, int fmt_opts) {
    return simple_format(obj, fmt_opts);
}

void class_TEMPLATE() {
    /* add classes */
    for (int i = 0; i < (int)G_N_ELEMENTS(cls_TEMPLATE); i++)
        class_add(&cls_TEMPLATE[i]);
}

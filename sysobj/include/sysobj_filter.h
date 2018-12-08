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

#ifndef _SYSOBJ_FILTER_H_
#define _SYSOBJ_FILTER_H_

#include "sysobj.h"

enum {
                               /*  match | no-match */
    SO_FILTER_NONE        = 0, /*     NC | NC       */
    SO_FILTER_EXCLUDE     = 1, /*      E | NC       */
    SO_FILTER_INCLUDE     = 2, /*      I | NC       */
    SO_FILTER_EXCLUDE_IIF = 5, /*      E | I        */
    SO_FILTER_INCLUDE_IIF = 6, /*      I | E        */

    SO_FILTER_IIF         = 4,
    SO_FILTER_MASK        = 7,
    SO_FILTER_STATIC      = 256, /* free the pspec, but not the filter */
};

typedef struct sysobj_filter {
    int type;
    gchar *pattern;
    GPatternSpec *pspec;
} sysobj_filter;

sysobj_filter *sysobj_filter_new(int type, gchar *pattern);
void sysobj_filter_free(sysobj_filter *f);
gboolean sysobj_filter_item_include(gchar *item, GSList *filters);
/* returns the new head of now-filtered items */
GSList *sysobj_filter_list(GSList *items, GSList *filters);

#endif

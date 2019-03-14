/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This file based on vendor.{h,c} from HardInfo
 * Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
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

#ifndef __VENDOR_H__
#define __VENDOR_H__

#include "gg_slist.h"

typedef GSList* vendor_list;
#define vendor_list_append(vl, v) g_slist_append(vl, (Vendor*)v)
#define vendor_list_concat(vl, ext) g_slist_concat(vl, ext)
vendor_list vendor_list_concat_va(int count, vendor_list vl, ...); /* count = -1 for NULL terminated list */
#define vendor_list_free(vl) g_slist_free(vl)
#define vendor_list_remove_duplicates(vl) gg_slist_remove_duplicates(vl)

typedef struct {
  char *match_string;
  int match_rule; /* 0 = ignore case, 1 = match case, 2 = exact */
  char *name;
  char *name_short;
  char *url;
  char *url_support;
  char *wikipedia; /* wikipedia page title (assumes en:, otherwise include langauge), usually more informative than the vendor's page */
  char *note;      /* a short stored comment */
  char *ansi_color;

  unsigned long file_line;
  unsigned long ms_length;
} Vendor;

void vendor_init(void);
void vendor_cleanup(void);
const Vendor *vendor_match(const gchar *id_str, ...); /* end list of strings with NULL */
vendor_list vendors_match(const gchar *id_str, ...);  /* end list of strings with NULL */
const gchar *vendor_get_name(const gchar *id_str);
const gchar *vendor_get_shortest_name(const gchar *id_str);
const gchar *vendor_get_url(const gchar *id_str);
void vendor_free(Vendor *v);

#endif /* __VENDOR_H__ */

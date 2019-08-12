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
vendor_list vendor_list_remove_duplicates_deep(vendor_list vl);

enum {
  VENDOR_MATCH_RULE_WORD_IGNORE_CASE   = 0,
  VENDOR_MATCH_RULE_WORD_MATCH_CASE    = 1,
  VENDOR_MATCH_RULE_EXACT              = 2,
  VENDOR_MATCH_RULE_WORD_PREFIX_IGNORE_CASE = 3,
  VENDOR_MATCH_RULE_WORD_PREFIX_MATCH_CASE  = 4,
  VENDOR_MATCH_RULE_WORD_SUFFIX_IGNORE_CASE = 5,
  VENDOR_MATCH_RULE_WORD_SUFFIX_MATCH_CASE  = 6,
  /* "ST" hits for "ST3600A" but not "AST" or "STMicro" or "STEC" */
  VENDOR_MATCH_RULE_NUM_PREFIX_IGNORE_CASE  = 7,
  VENDOR_MATCH_RULE_NUM_PREFIX_MATCH_CASE   = 8,
};

typedef struct {
  char *match_string;
  int match_rule; /* VENDOR_MATCH_RULE_* enum */
  char *name;
  char *name_short;
  char *url;
  char *url_support;
  char *wikipedia; /* wikipedia page title (assumes en:, otherwise include langauge), usually more informative than the vendor's page */
  char *note;      /* a short stored comment */
  char *ansi_color;

  unsigned long file_line;
  unsigned long ms_length;
  unsigned long weight;
} Vendor;

void vendor_init(void);
void vendor_cleanup(void);
/* end list of strings with NULL */
const Vendor *vendor_match(const gchar *id_str, ...)
  __attribute__((sentinel));
vendor_list vendors_match(const gchar *id_str, ...)
  __attribute__((sentinel));
const gchar *vendor_get_name(const gchar *id_str);
const gchar *vendor_get_shortest_name(const gchar *id_str);
const gchar *vendor_get_url(const gchar *id_str);
void vendor_free(Vendor *v);

int vendor_cmp_deep(const Vendor *a, const Vendor *b);
vendor_list vendors_match_core(const gchar *str, int limit);

extern gboolean vendor_die_on_error;

#endif /* __VENDOR_H__ */

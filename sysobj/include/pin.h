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

#ifndef _PIN_H_
#define _PIN_H_

#include "sysobj.h"

typedef struct pin {
    double update_interval; /* 0 = static, will never be re-read */
    double last_update;
    sysobj *obj;
    sysobj_data **history;
    const sysobj_data *min; /* in history */
    const sysobj_data *max; /* in history */
    /* -1 = not keeping history */
    /*  0 = not yet determined */
    /*  1 = keeping, have f_compare() */
    /* 10 = keeping, guessing compare base 10 number */
    /* 16 = keeping, guessing compare base 16 number */
    int history_status;
    uint64_t history_len;
    uint64_t history_max_len;
    uint64_t history_mem;   /* size of allocated block in num of structs */
    int group; /* -1 no group, or index into pin_list groups */
} pin;

typedef struct pin_list {
    GSList *list;
    GTimer *timer;
    double shortest_interval;
    double longest_interval;
    GSList *groups;
} pin_list;

pin *pin_new();
pin *pin_new_sysobj(sysobj *obj); /* takes ownership of obj */
pin *pin_dup(const pin *src);
void pin_update(pin *p, gboolean force);
void pin_free(void *p); /* void for glib */

pin_list *pins_new();
int pins_add_from_fn(pin_list *pl, const gchar *base, const gchar *name);
void pins_refresh(pin_list *pl);
pin *pins_get_nth(pin_list *pl, int i);
pin *pins_find_by_path(pin_list *pl, const gchar *path);
const pin *pins_pin_if_updated_since(pin_list *pl, int pi, double seconds_ago); /* NULL if unchanged, or pin* to avoid another pins_get_nth() */
const sysobj_data *pins_history_data_when(pin_list *pl, pin *p, double seconds_ago);
void pins_clear(pin_list *pl);
void pins_free(pin_list *pl);

pin_list *pins_new_from_kv(const gchar *kv_data);
gchar *pins_to_kv(pin_list *pl);

#endif

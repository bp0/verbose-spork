
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
} pin;

typedef struct pin_list {
    GSList *list;
    GTimer *timer;
    double shortest_interval;
    double longest_interval;
} pin_list;

pin *pin_new();
pin *pin_new_sysobj(sysobj *obj); /* takes ownership of obj */
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

#endif

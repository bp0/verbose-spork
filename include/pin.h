
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
pin *pin_new_sysobj(sysobj *obj);
void pin_update(pin *p, gboolean force);
void pin_free(void *p); /* void for glib */

pin_list *pins_new();
void pins_add_from_fn(pin_list *pl, const gchar *base, const gchar *name);
void pins_refresh(pin_list *pl);
void pins_free(pin_list *pl);

#endif

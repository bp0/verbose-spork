
#include "pin.h"

#define PIN_HIST_BLOCK_SIZE 100 /* num of structs to allocate */

pin *pin_new() {
    pin *p = g_new0(pin, 1);
    return p;
}

pin *pin_new_sysobj(sysobj *obj) {
    if (obj) {
        pin *p = pin_new();
        p->obj = obj;
        if (p->obj->cls) {
            if (p->obj->cls->f_update_interval)
                p->update_interval = p->obj->cls->f_update_interval(p->obj);
        }
        if (!p->update_interval) {
            /* static, only read once */
            pin_update(p, TRUE);
        }
        return p;
    }
    return NULL;
}

void pin_free(void *ptr) {
    uint64_t i = 0;
    pin *p = (pin *)ptr;
    if (p) {
        if (p->history) {
            for (i = 0; i < p->history_len; i++)
                g_free(p->history[i]);
            g_free(p->history);
        }
        sysobj_free(p->obj);
        g_free(p);
    }
}

void pin_update(pin *p, gboolean force) {
    if (p->obj) {
        if (p->update_interval || force) {
            const sysobj_class *c = p->obj->cls;
            sysobj_read_data(p->obj);
            if (p->history) {
                if (p->history_len == p->history_mem) {
                    p->history_mem += PIN_HIST_BLOCK_SIZE;
                    p->history = g_renew(sysobj_data*, p->history, p->history_mem);
                }
                p->history_len++;
            } else {
                p->history_mem = PIN_HIST_BLOCK_SIZE;
                p->history = g_new0(sysobj_data*, p->history_mem);
                p->history_len = 1;
            }
            uint64_t i = p->history_len-1;
            p->history[i] = sysobj_data_dup(&p->obj->data);
            if (c && c->f_compare) {
                if ( c->f_compare(p->min, p->history[i]) >= 0 )
                    p->min = p->history[p->history_len-1];
                if ( c->f_compare(p->max, p->history[i]) <= 0 )
                    p->max = p->history[p->history_len-1];
            }
        }
    }
}

pin_list *pins_new() {
    pin_list *pl = g_new0(pin_list, 1);
    pl->timer = g_timer_new();
    g_timer_start(pl->timer);
    return pl;
}

void pin_add_from_fn(pin_list *pl, const gchar *base, const gchar *name) {
    sysobj *obj = sysobj_new_from_fn(base, name);
    if (obj->exists) {
        pin *p = pin_new_sysobj(obj);
        pl->list = g_slist_append(pl->list, p);
        if (p->update_interval) {
            if (p->update_interval > pl->longest_interval)
                pl->longest_interval = p->update_interval;
            if (p->update_interval < pl->shortest_interval)
                pl->shortest_interval = p->update_interval;
        }
    } else
        sysobj_free(obj);
}

void pins_refresh(pin_list *pl) {
    GSList *l = pl->list;
    while (l) {
        pin *p = l->data;
        if (p->update_interval) {
            double elapsed = g_timer_elapsed(pl->timer, NULL);
            if ( p->last_update + p->update_interval > elapsed ) {
                p->last_update = elapsed;
                pin_update(p, FALSE);
            }
        }
        l = l->next;
    }
}

void pins_free(pin_list *pl) {
    if (pl) {
        g_timer_destroy(pl->timer);
        g_slist_free_full(pl->list, pin_free);
        g_free(pl);
    }
}


#include "pin.h"

#define PIN_HIST_BLOCK_SIZE 100 /* num of structs to allocate */
#define PIN_HIST_MAX_DEFAULT 10  /* max num of samples(structs) to keep */

pin *pin_new() {
    pin *p = g_new0(pin, 1);
    return p;
}

pin *pin_new_sysobj(sysobj *obj) {
    if (obj) {
        pin *p = pin_new();
        p->obj = obj;
        p->update_interval = sysobj_update_interval(p->obj);
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

const pin *pins_pin_if_updated_since(pin_list *pl, int pi, double seconds_ago) {
    if (pl) {
        double elapsed = g_timer_elapsed(pl->timer, NULL);
        pin *p = pins_get_nth(pl, pi);
        if (p && (elapsed - p->last_update < seconds_ago) )
                return p;
    }
    return NULL;
}

void pin_update(pin *p, gboolean force) {
    f_compare_sysobj_data *compare_func = NULL;
    if (p->obj) {
        if (p->update_interval || force) {
            const sysobj_class *c = p->obj->cls;
            sysobj_read_data(p->obj);
            if (!p->history_status) {
                if (c && c->f_compare)
                    p->history_status = 1;
                else if (p->obj->data.maybe_num) {
                    p->history_status = p->obj->data.maybe_num;
                } else
                    p->history_status = -1;
                p->history_max_len = PIN_HIST_MAX_DEFAULT;
            }

            if (p->history_status > 0) {
                if (p->history_status > 1) {
                    switch(p->obj->data.maybe_num) {
                        case 0:
                            p->history_status = -1; /* previously guessed, but apparently wrong */
                            break;
                        case 16:
                            p->history_status = 16; /* upgrade to hex if a hex digit is seen */
                            break;
                        case 10:
                        default:
                            break;
                    }
                }

                if (c && c->f_compare)
                    compare_func = c->f_compare;
                else if (p->history_status == 10)
                    compare_func = compare_str_base10;
                else if (p->history_status == 16)
                    compare_func = compare_str_base16;

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
                if (compare_func) {
                    if (!p->min || compare_func(p->min, p->history[i]) >= 0 )
                        p->min = p->history[p->history_len-1];
                    if ( compare_func(p->max, p->history[i]) <= 0 )
                        p->max = p->history[p->history_len-1];
                }
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

pin *pins_find_by_path(pin_list *pl, const gchar *path) {
    if (pl && path) {
        GSList *l = pl->list;
        while (l) {
            pin *p = l->data;
            if (g_strcmp0(p->obj->path, path) == 0)
                return p;
            l = l->next;
        }
    }
    return NULL;
}

int pins_add_from_fn(pin_list *pl, const gchar *base, const gchar *name) {
    sysobj *obj = sysobj_new_from_fn(base, name);
    if (obj->exists) {
        /* check if already in list */
        pin *p = pins_find_by_path(pl, obj->path);
        if (p) {
            sysobj_free(obj);
            return -1;
        }

        /* if not, then add */
        p = pin_new_sysobj(obj);
        pl->list = g_slist_append(pl->list, p);
        if (p->update_interval) {
            if (p->update_interval > pl->longest_interval)
                pl->longest_interval = p->update_interval;
            if (p->update_interval < pl->shortest_interval
                || pl->shortest_interval == 0)
                pl->shortest_interval = p->update_interval;
        }
        return g_slist_length(pl->list) - 1;
    } else
        sysobj_free(obj);
    return -1;
}

void pins_refresh(pin_list *pl) {
    //DEBUG("Refreshing list %llx", (long long int)pl);
    int updated = 0;
    double elapsed = g_timer_elapsed(pl->timer, NULL);
    GSList *l = pl->list;
    while (l) {
        pin *p = l->data;
        if (p->update_interval) {
            /* DEBUG("obj: %s, ui: %lf, lu: %lf el: %lf %s", p->obj->name, p->update_interval, p->last_update, elapsed,
                ((p->last_update + p->update_interval) < elapsed) ? "update" : "no-update" ); */
            if ( p->last_update + p->update_interval < elapsed ) {
                p->last_update = elapsed;
                pin_update(p, FALSE);
                updated++;
            }
        }
        l = l->next;
    }
    //DEBUG("... %d updated.", updated);
}

pin *pins_get_nth(pin_list *pl, int i) {
    pin *p = NULL;
    if (pl) {
        GSList *li = g_slist_nth(pl->list, i);
        if (li)
            p = (pin*)li->data;
    }
    return p;
}

void pins_clear(pin_list *pl) {
    g_slist_free_full(pl->list, pin_free);
    pl->list = NULL;
    g_timer_start(pl->timer);
    pl->shortest_interval = 0;
    pl->longest_interval = 0;
}

void pins_free(pin_list *pl) {
    if (pl) {
        g_timer_destroy(pl->timer);
        g_slist_free_full(pl->list, pin_free);
        g_free(pl);
    }
}

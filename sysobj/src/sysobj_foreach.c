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

#include "sysobj_foreach.h"
#include <unistd.h> /* for usleep() */

static GList* __push_if_uniq(GList *l, const gchar *base, const gchar *name, long unsigned int *length) {
    gchar *path = name
        ? g_strdup_printf("%s/%s", base, name)
        : g_strdup(base);
    GList *a = NULL, *last = a;
    for(a = l; a; a = a->next) {
        if (SEQ((gchar*)a->data, path) ) {
            g_free(path);
            return l;
        }
        last = a;
    }
    if (length) (*length)++;
    if (last)
        last = g_list_append(last, path);
    else
        l = g_list_append(l, path);
    return l;
}

static GList* __shift(GList *l, gchar **s) {
    if (!l) return l;
    *s = (gchar*)l->data;
    return g_list_delete_link(l, l);
}

static GList* __pop(GList *l, gchar **s) {
    GList *last = g_list_last(l);
    if (last)
        *s = (gchar*)last->data;
    return g_list_delete_link(l, last);
}

typedef struct {
    GList *to_search;
    GMutex lock;
    GMutex lock_stats;
    gboolean stop;

    GSList *threads;

    GSList *filters;
    f_sysobj_foreach callback;
    gpointer user_data;

    sysobj_foreach_stats stats;
} mt_state;

static void mt_state_init(mt_state *s) {
    if (!s) return;
    s->stop = FALSE;
    s->to_search = NULL;
    memset(&s->stats, 0, sizeof(sysobj_foreach_stats) );
    s->stats.start_time = sysobj_elapsed();
    g_mutex_init(&s->lock);
    g_mutex_init(&s->lock_stats);
}

static void mt_state_clear(mt_state *s) {
    g_list_free_full(s->to_search, g_free);
    g_mutex_clear(&s->lock);
    g_mutex_clear(&s->lock_stats);
    g_slist_free(s->threads);
}

static void mt_update(mt_state *s, guint increment) {
    g_mutex_lock(&s->lock_stats);
    s->stats.searched += increment;
    s->stats.rate = (double)s->stats.searched / (sysobj_elapsed() - s->stats.start_time);
    g_mutex_unlock(&s->lock_stats);
}

#define WAIT_TIME 10000
#define WAIT_TOO_MUCH 2000000

static gpointer _sysobj_foreach_thread_main(mt_state *s) {
    gchar *path = NULL;
    while(1) {
        if (path) {
            g_free(path);
            path = NULL;
        }

        if (s->stop || s->stats.total_wait > WAIT_TOO_MUCH) {
            g_mutex_lock(&s->lock_stats);
            if (!s->stats.end_type) {
                if (s->stop)
                    s->stats.end_type = SO_FOREACH_END_INT;
                else if (s->stats.total_wait > WAIT_TOO_MUCH)
                    s->stats.end_type = SO_FOREACH_END_EXH;
            }
            g_mutex_unlock(&s->lock_stats);
            goto foreach_main_finish;
        }

        g_mutex_lock(&s->lock);
        //s->to_search = __shift(s->to_search, &path); // breadth-first
        s->to_search = __pop(s->to_search, &path); // depth-first
        g_mutex_unlock(&s->lock);

        if (!path) {
            if (s->stats.threads == 1) {
                /* so that we end with SO_FOREACH_END_EXH */
                s->stats.total_wait += WAIT_TOO_MUCH + 1;
                continue;
            }
            g_mutex_lock(&s->lock_stats);
            s->stats.total_wait += WAIT_TIME;
            g_mutex_unlock(&s->lock_stats);
            //DEBUG("omg had to wait (total_wait: %lluus)", (long long unsigned int)s->stats.total_wait);
            usleep(WAIT_TIME);
            continue;
        }

        if (0)
        printf("[%p](rate: %0.2lf/s) to_search:%lu searched:%lu now: %s\n",
            g_thread_self(), s->stats.rate, s->stats.queue_length, s->stats.searched, path);

        sysobj *obj = sysobj_new_fast(path);
        if (!obj) { g_free(path); continue; }
        if (s->filters
            && !sysobj_filter_item_include(obj->path, s->filters) ) {
                sysobj_free(obj);
                g_mutex_lock(&s->lock_stats);
                s->stats.filtered++;
                g_mutex_unlock(&s->lock_stats);
                continue;
            }

        /* callback */
        if ( s->callback(obj, s->user_data, &s->stats) == SYSOBJ_FOREACH_STOP ) {
            s->stop = TRUE;
            sysobj_free(obj);
            continue;
        }

        if (obj->data.is_dir && !obj->req_is_link)
            sysobj_read(obj, FALSE);

        g_mutex_lock(&s->lock);
        mt_update(s, 1);
        /* queue children */
        const GSList *lc = obj->data.childs;
        for (lc = obj->data.childs; lc; lc = lc->next) {
            s->to_search = __push_if_uniq(s->to_search, obj->path_req, (gchar*)lc->data, &s->stats.queue_length);
        }
        g_mutex_unlock(&s->lock);

        sysobj_free(obj);
    }
    /* never arrives here */

foreach_main_finish:
    if (s->stats.threads != 1)
        free_auto_free_thread_final();
    return NULL;
}

static void sysobj_foreach_mt(const gchar *root_path, GSList *filters, f_sysobj_foreach callback, gpointer user_data, int max_threads) {
    GSList *l = NULL;
    mt_state state = {.filters = filters, .callback = callback, .user_data = user_data };
    mt_state_init(&state);
    state.stats.threads = g_get_num_processors();
    if (max_threads && state.stats.threads > max_threads)
        state.stats.threads = max_threads;
    if (root_path)
        state.to_search = __push_if_uniq(state.to_search, root_path, NULL, &state.stats.queue_length);
    else {
        state.to_search = __push_if_uniq(state.to_search, ":/", NULL, &state.stats.queue_length);
        state.to_search = __push_if_uniq(state.to_search, "/sys", NULL, &state.stats.queue_length);
        state.to_search = __push_if_uniq(state.to_search, "/proc", NULL, &state.stats.queue_length);
    }

    if (state.stats.threads == 1) {
        _sysobj_foreach_thread_main(&state);
    } else {
        for (int t = 0; t < state.stats.threads; t++) {
            GThread *nt = g_thread_new(NULL, (GThreadFunc)_sysobj_foreach_thread_main, &state);
            state.threads = g_slist_append(state.threads, nt);
        }

        for(l = state.threads; l; l = l->next)
            g_thread_join(l->data);
    }

    char end_type_str[16] = "";
    switch(state.stats.end_type) {
        case SO_FOREACH_END_INT:
            strcpy(end_type_str, "INT");
            break;
        case SO_FOREACH_END_EXH:
            strcpy(end_type_str, "EXH");
            break;
        case SO_FOREACH_END_UND:
            strcpy(end_type_str, "UND");
            break;
        default:
            sprintf(end_type_str, "?:%d", state.stats.end_type);
            break;
    }

    DEBUG("sysobj_foreach done [%s] threads: %lu, searched: %llu, time: %0.2lfs, wait: %0.4lfs (in all threads), rate: %0.2lf/s",
        end_type_str,
        (long unsigned)state.stats.threads,
        (long long unsigned)state.stats.searched,
        sysobj_elapsed() - state.stats.start_time,
        (double)state.stats.total_wait / 1000000,
        state.stats.rate);

    mt_state_clear(&state);
}

void sysobj_foreach(GSList *filters, f_sysobj_foreach callback, gpointer user_data, int opts) {
    gboolean use_mt = !!(opts & SO_FOREACH_MT);
    sysobj_foreach_mt(NULL, filters, callback, user_data, use_mt ? 0 : 1);
}

void sysobj_foreach_from(const gchar *root_path, GSList *filters, f_sysobj_foreach callback, gpointer user_data, int opts) {
    gboolean use_mt = !!(opts & SO_FOREACH_MT);
    sysobj_foreach_mt(root_path, filters, callback, user_data, use_mt ? 0 : 1);
}


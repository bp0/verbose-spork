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

static GList* __push_if_uniq(GList *l, gchar *base, gchar *name, long unsigned int *length) {
    gchar *path = name
        ? g_strdup_printf("%s/%s", base, name)
        : g_strdup(base);
    GList *a = NULL, *last = a;
    for(a = l; a; a = a->next) {
        if (!g_strcmp0((gchar*)a->data, path) ) {
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

static gboolean _has_symlink_loop(const gchar *path) {
    GList *targets = NULL;
    sysobj *obj = sysobj_new_from_fn(path, NULL);
    sysobj *next = NULL;
    while(obj) {
        if ( g_list_find_custom(targets, obj->path, (GCompareFunc)g_strcmp0) )
            goto loop_check_fail;

        targets = g_list_prepend(targets, g_strdup(obj->path) );

        /* request parent */
        next = sysobj_parent(obj);
        sysobj_free(obj);
        obj = next;
    }
    g_list_free_full(targets, g_free);
    return FALSE;

loop_check_fail:
    sysobj_free(obj);
    g_list_free_full(targets, g_free);
    return TRUE;
}

static void sysobj_foreach_st(GSList *filters, f_sysobj_foreach callback, gpointer user_data) {
    double rate = 0.0, start_time = sysobj_elapsed();
    guint searched = 0;
    GList *to_search = NULL;
    to_search = __push_if_uniq(to_search, ":/", NULL, NULL);
    gchar *path = NULL;
    while( g_list_length(to_search) ) {
        to_search = __shift(to_search, &path);
        rate = (double)searched / (sysobj_elapsed() - start_time);
        printf("(rate: %0.2lf/s) to_search:%d searched:%d now: %s\n", rate, g_list_length(to_search), searched, path );
        if (_has_symlink_loop(path) ) { g_free(path); continue; }

        sysobj *obj = sysobj_new_from_fn(path, NULL);
        if (!obj) { g_free(path); continue; }
        if (filters
            && !sysobj_filter_item_include(obj->path, filters) ) {
                sysobj_free(obj);
                g_free(path);
                continue;
            }

        /* callback */
        if ( callback(obj, user_data, NULL) == SYSOBJ_FOREACH_STOP ) {
            sysobj_free(obj);
            g_free(path);
            break;
        }
        searched++;

        /* scan children */
        if (obj->data.is_dir) {
            sysobj_read(obj, FALSE);
            const GSList *lc = obj->data.childs;
            for (lc = obj->data.childs; lc; lc = lc->next) {
                to_search = __push_if_uniq(to_search, obj->path_req, (gchar*)lc->data, NULL);
            }
        }

        sysobj_free(obj);
        g_free(path);
    }
    g_list_free_full(to_search, g_free);
}

typedef struct {
    GList *to_search;
    GMutex lock;
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
}

static void mt_state_clear(mt_state *s) {
    g_list_free_full(s->to_search, g_free);
    g_mutex_clear(&s->lock);
    g_slist_free(s->threads);
}

static void mt_update(mt_state *s, guint increment) {
    s->stats.searched += increment;
    s->stats.rate = (double)s->stats.searched / (sysobj_elapsed() - s->stats.start_time);
}

static gpointer _thread_main(mt_state *s) {
    gchar *path = NULL;
    while(1) {
        g_mutex_lock(&s->lock);
        s->to_search = __shift(s->to_search, &path);
        //s->to_search = __pop(s->to_search, &path);
        g_mutex_unlock(&s->lock);

        if (s->stop) {
            g_free(path);
            return NULL;
        }

        if (!path) { printf("omg had to wait\n"); usleep(10000); continue; }

        printf("[0x%016llx](rate: %0.2lf/s) to_search:%lu searched:%lu now: %s\n",
            (long long unsigned)g_thread_self(), s->stats.rate, s->stats.queue_length, s->stats.searched, path);

        if (_has_symlink_loop(path) ) { g_free(path); continue; }

        sysobj *obj = sysobj_new_fast(path);
        if (!obj) { g_free(path); continue; }
        if (s->filters
            && !sysobj_filter_item_include(obj->path, s->filters) ) {
                sysobj_free(obj);
                g_free(path);
                continue;
            }

        /* callback */
        if ( s->callback(obj, s->user_data, &s->stats) == SYSOBJ_FOREACH_STOP ) {
            s->stop = TRUE;
            sysobj_free(obj);
            g_free(path);
            return NULL;
        }

        if (obj->data.is_dir)
            sysobj_read(obj, FALSE);

        g_mutex_lock(&s->lock);
        mt_update(s, 1);
        /* queue children */
        const GSList *lc = obj->data.childs;
        for (lc = obj->data.childs; lc; lc = lc->next) {
            s->to_search = __push_if_uniq(s->to_search, obj->path_req, (gchar*)lc->data, &s->stats.queue_length);
        }
        g_mutex_unlock(&s->lock);

        if (obj->data.was_read && !obj->data.is_dir) exit(0);
        sysobj_free(obj);
        g_free(path);
    }
    /* never arrives here */
}

static void sysobj_foreach_mt(GSList *filters, f_sysobj_foreach callback, gpointer user_data) {
    GSList *l = NULL;
    mt_state state = {.filters = filters, .callback = callback, .user_data = user_data };
    mt_state_init(&state);
    state.stats.threads = g_get_num_processors();
    state.to_search = __push_if_uniq(state.to_search, ":/", NULL, &state.stats.queue_length);

    for (int t = 0; t < state.stats.threads; t++) {
        GThread *nt = g_thread_new(NULL, (GThreadFunc)_thread_main, &state);
        state.threads = g_slist_append(state.threads, nt);
    }

    for(l = state.threads; l; l = l->next)
        g_thread_join(l->data);

    mt_state_clear(&state);
}

void sysobj_foreach(GSList *filters, f_sysobj_foreach callback, gpointer user_data, int opts) {
    gboolean use_mt = !(opts & SO_FOREACH_ST);

    if (use_mt)
        sysobj_foreach_mt(filters, callback, user_data);
    else
        sysobj_foreach_st(filters, callback, user_data);
}

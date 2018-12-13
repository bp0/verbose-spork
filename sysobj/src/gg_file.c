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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "gg_file.h"

#define GFC_PAGE_SIZE 4096
#define GFC_MAX_BLOCK_TIME_US 100000 /* 100000us = 100ms = 0.1s, right? */
#define GFC_WAIT_US 1000
gboolean gg_file_get_contents_non_blocking(const gchar *file, gchar **contents, gsize *size, int *err) {
    int fd;
    int rlen = 0, errn = 0;
    unsigned long long int sleep = 0, total_sleep = 0;

    gchar *buff = NULL, *loc = NULL, *tmp = NULL;
    gsize pages = 1, ps = 0, fs = 0;

    fd = open(file, O_RDONLY | O_NONBLOCK );
    if (fd == -1) {
        if (err) *err = errno;
        return FALSE;
    }

    loc = buff = g_malloc( pages * GFC_PAGE_SIZE );
    while((rlen = read(fd, (char*)loc, (GFC_PAGE_SIZE - ps))) != 0) {
        if (rlen == -1) {
            errn = errno;
            if (errn == EWOULDBLOCK) {
                if (sleep > GFC_MAX_BLOCK_TIME_US)
                    break;
                usleep(GFC_WAIT_US);
                sleep += GFC_WAIT_US;
                total_sleep += GFC_WAIT_US;
                continue;
            }
            break;
        }
        sleep = 0;

        fs += rlen;
        ps += rlen;

        if (ps == GFC_PAGE_SIZE) {
            pages++;
            buff = g_realloc(buff, pages * GFC_PAGE_SIZE);
            loc = buff + ((pages-1) * GFC_PAGE_SIZE);
            ps = 0;
        } else
            loc = buff + fs;
    }
    close(fd);

    tmp = g_malloc0(fs + 1); /* the +1 should be a null */
    memmove(tmp, buff, fs);
    g_free(buff);
    buff = tmp;

    if (size) *size = fs;
    if (contents) *contents = buff;
    if (err) *err = 0;
    return TRUE;
}

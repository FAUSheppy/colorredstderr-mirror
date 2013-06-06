/*
 * Debug functions.
 *
 * Copyright (C) 2013  Simon Ruderich
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEBUG_H
#define DEBUG_H 1

static void debug(const char *format, ...) {
    char buffer[1024];

    /* If the file doesn't exist, do nothing. Prevents writing log files in
     * unexpected places. The user must create the file manually. */
    int fd = open(DEBUG_FILE, O_WRONLY | O_APPEND);
    if (fd == -1) {
        return;
    }

    va_list ap;
    va_start(ap, format);
    int written = vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    /* Overflow. */
    if ((size_t)written >= sizeof(buffer)) {
        written = sizeof(buffer) - 1;
    }

    /* Make sure these functions are loaded. */
    DLSYM_FUNCTION(real_write, "write");
    DLSYM_FUNCTION(real_close, "close");

    static int first_call = 0;
    if (!first_call++) {
        char nl = '\n';
        real_write(fd, &nl, 1);
    }
    real_write(fd, buffer, (size_t)written);
    real_close(fd);
}

#endif

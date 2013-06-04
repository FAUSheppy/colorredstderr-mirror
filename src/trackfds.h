/*
 * Utility functions to track file descriptors.
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

#ifndef TRACKFDS_H
#define TRACKFDS_H 1

/* List of tracked file descriptors. */
static int *tracked_fds;
/* Current number of items in the list. */
static size_t tracked_fds_count;
/* Allocated items, used to reduce realloc()s. */
static size_t tracked_fds_space;


#ifdef DEBUG
static void tracked_fds_debug(void) {
    debug("    tracked_fds: %d/%d\t\t[%d]\n", tracked_fds_count,
                                              tracked_fds_space,
                                              getpid());
    size_t i;
    for (i = 0; i < tracked_fds_count; i++) {
        debug("    tracked_fds[%d]: %d\n", i, tracked_fds[i]);
    }
}
#endif

/* Load tracked file descriptors from the environment. The environment is used
 * to pass the information to child processes.
 *
 * ENV_NAME_FDS has the following format: Each descriptor as string followed
 * by a comma; there's a trailing comma. Example: "2,4,". */
static void init_from_environment(void) {
#ifdef DEBUG
    debug("init_from_environment()\t\t[%d]\n", getpid());
#endif
    const char *env;

    initialized = 1;
    tracked_fds_count = 0;

    /* If ENV_NAME_FORCE_WRITE is set and not empty, allow writes to a non-tty
     * device. Use with care! Mainly used for the test suite. */
    env = getenv(ENV_NAME_FORCE_WRITE);
    if (env && env[0] != '\0') {
        force_write_to_non_tty = 1;
    }

    env = getenv(ENV_NAME_FDS);
    if (!env) {
        return;
    }
    /* Environment is read-only. */
    char *env_copy = strdup(env);
    if (!env_copy) {
        return;
    }

    char *x;

    size_t count = 0;
    for (x = env_copy; *x; x++) {
        if (*x == ',') {
            count++;
        }
    }
    tracked_fds_space = count + TRACKFDS_REALLOC_STEP;

    tracked_fds = malloc(tracked_fds_space * sizeof(*tracked_fds));
    if (!tracked_fds) {
        free(env_copy);
        return;
    }

    size_t i = 0;

    /* Parse file descriptor numbers from environment string and store them as
     * integers in tracked_fds. */
    char *last;
    for (x = env_copy, last = env_copy; *x; x++) {
        if (*x != ',') {
            continue;
        }
        /* ',' at the beginning or double ',' - ignore. */
        if (x == last) {
            last = x + 1;
            continue;
        }

        if (i == count) {
            break;
        }

        *x = 0;
        tracked_fds[i++] = atoi(last);

        last = x + 1;
    }

    tracked_fds_count = count;

    free(env_copy);

#ifdef DEBUG
    tracked_fds_debug();
#endif
}

static void update_environment(void) {
#ifdef DEBUG
    debug("update_environment()\t\t[%d]\n", getpid());
#endif

    /* An integer (32-bit) has at most 10 digits, + 1 for the comma after each
     * number. Bigger file descriptors (which shouldn't occur in reality) are
     * skipped. */
    char env[tracked_fds_count * (10 + 1) + 1 /* to fit '\0' */ ];
    env[0] = 0;

    char *x = env;

    size_t i;
    for (i = 0; i < tracked_fds_count; i++) {
        int length = snprintf(x, 10 + 1, "%d", tracked_fds[i]);
        if (length >= 10 + 1) {
            /* Integer too bit to fit the buffer, skip it. */
            continue;
        }

        /* Write comma after number. */
        x += length;
        *x++ = ',';
        /* Make sure the string is always zero terminated. */
        *x = 0;
    }

#if 0
    debug("    setenv('%s', '%s', 1)\n", ENV_NAME_FDS, env);
#endif

    setenv(ENV_NAME_FDS, env, 1 /* overwrite */);
}



static void tracked_fds_add(int fd) {
    if (tracked_fds_count >= tracked_fds_space) {
        size_t new_space = tracked_fds_space + TRACKFDS_REALLOC_STEP;
        int *tmp = realloc(tracked_fds, sizeof(*tracked_fds) * new_space);
        if (!tmp) {
            /* We can do nothing, just ignore the error. We made sure not to
             * destroy our state, so the new descriptor is ignored without any
             * other consequences. */
#ifdef DEBUG
            debug("realloc(tracked_fds, %zu) failed! [%d]\n",
                  sizeof(*tracked_fds) * new_space, getpid());
#endif
            return;
        }
        tracked_fds = tmp;
        tracked_fds_space = new_space;
    }

    tracked_fds[tracked_fds_count++] = fd;

#ifdef DEBUG
    debug("tracked_fds_add(): %-3d\t\t[%d]\n", fd, getpid());
    tracked_fds_debug();
#endif
}
static int tracked_fds_remove(int fd) {
    size_t i;
    for (i = 0; i < tracked_fds_count; i++) {
        if (fd != tracked_fds[i]) {
            continue;
        }

        memmove(tracked_fds + i, tracked_fds + i + 1,
                sizeof(*tracked_fds) * (tracked_fds_count - i - 1));
        tracked_fds_count--;

#ifdef DEBUG
        debug("tracked_fds_remove(): %-3d\t[%d]\n", fd, getpid());
        tracked_fds_debug();
#endif

        /* Found. */
        return 1;
    }

    /* Not found. */
    return 0;
}
static int tracked_fds_find(int fd) {
    size_t i;
    for (i = 0; i < tracked_fds_count; i++) {
        if (fd == tracked_fds[i]) {
            return 1;
        }
    }
    return 0;
}

#endif

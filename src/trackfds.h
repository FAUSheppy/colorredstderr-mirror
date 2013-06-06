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

/* Array of tracked file descriptors. Used for fast lookups for the normally
 * used file descriptors (0 <= fd < TRACKFDS_STATIC_COUNT). */
static int tracked_fds[TRACKFDS_STATIC_COUNT];

/* List of tracked file descriptors >= TRACKFDS_STATIC_COUNT. */
static int *tracked_fds_list;
/* Current number of items in the list. */
static size_t tracked_fds_list_count;
/* Allocated items, used to reduce realloc()s. */
static size_t tracked_fds_list_space;


#ifdef DEBUG
static void tracked_fds_debug(void) {
    size_t i;

    for (i = 0; i < TRACKFDS_STATIC_COUNT; i++) {
        if (tracked_fds[i]) {
            debug("    tracked_fds[%d]: %d\n", i, tracked_fds[i]);
        }
    }
    debug("    tracked_fds_list: %d/%d\t[%d]\n", tracked_fds_list_count,
                                                 tracked_fds_list_space,
                                                 getpid());
    for (i = 0; i < tracked_fds_list_count; i++) {
        debug("    tracked_fds_list[%d]: %d\n", i, tracked_fds_list[i]);
    }
}
#endif

static int init_tracked_fds_list(size_t count) {
    /* Reduce reallocs. */
    count += TRACKFDS_REALLOC_STEP;

    tracked_fds_list = malloc(count * sizeof(*tracked_fds_list));
    if (!tracked_fds_list) {
#ifdef DEBUG
        warning("malloc(tracked_fds_list, %d) failed [%d]\n",
                count * sizeof(*tracked_fds_list), getpid());
#endif
        return 0;
    }

    tracked_fds_list_space = count;
    return 1;
}

/* Load tracked file descriptors from the environment. The environment is used
 * to pass the information to child processes.
 *
 * ENV_NAME_FDS has the following format: Each descriptor as string followed
 * by a comma; there's a trailing comma. Example: "2,4,". */
static void init_from_environment(void) {
#ifdef DEBUG
    debug("init_from_environment()\t\t[%d]\n", getpid());
#endif
    char const *env;

    initialized = 1;
    tracked_fds_list_count = 0;

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
    char env_copy[strlen(env) + 1];
    strcpy(env_copy, env);

    char *x;

    size_t count = 0;
    for (x = env_copy; *x; x++) {
        if (*x == ',') {
            count++;
        }
    }

    size_t i = 0;

    /* Parse file descriptor numbers from environment string and store them as
     * integers in tracked_fds and tracked_fds_list. */
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

        int fd = atoi(last);
        if (fd < TRACKFDS_STATIC_COUNT) {
            tracked_fds[fd] = 1;
        } else {
            if (!tracked_fds_list) {
                /* Pessimistic count estimate, but allocating a few more
                 * elements doesn't hurt. */
                if (!init_tracked_fds_list(count)) {
                    /* Couldn't allocate memory, skip this entry. */
                    warning("foo\n");
                    goto next;
                }
            }
            tracked_fds_list[i++] = fd;
#ifdef DEBUG
            debug("  large fd: %d\n", fd);
#endif
        }

next:
        last = x + 1;
    }

    tracked_fds_list_count = i;

#ifdef DEBUG
    tracked_fds_debug();
#endif
}

static char *update_environment_buffer_entry(char *x, int fd) {
    int length = snprintf(x, 10 + 1, "%d", fd);
    if (length >= 10 + 1) {
        /* Integer too big to fit the buffer, skip it. */
#ifdef DEBUG
        warning("update_environment_buffer_entry(): truncated fd: %d [%d]\n",
                fd, getpid());
#endif
        return x;
    }

    /* Write comma after number. */
    x += length;
    *x++ = ',';
    /* Make sure the string is always zero terminated. */
    *x = 0;

    return x;
}
static void update_environment_buffer(char *x) {
    size_t i;
    for (i = 0; i < TRACKFDS_STATIC_COUNT; i++) {
        if (tracked_fds[i]) {
            x = update_environment_buffer_entry(x, (int)i);
        }
    }
    for (i = 0; i < tracked_fds_list_count; i++) {
        x = update_environment_buffer_entry(x, tracked_fds_list[i]);
    }
}
inline static size_t update_environment_buffer_size(void) {
    /* Use the maximum count (TRACKFDS_STATIC_COUNT) of used descriptors
     * because it's simple and small enough not to be a problem.
     *
     * An integer (32-bit) has at most 10 digits, + 1 for the comma after each
     * number. Bigger file descriptors (which shouldn't occur in reality) are
     * skipped. */
    return (TRACKFDS_STATIC_COUNT + tracked_fds_list_count)
               * (10 + 1) + 1 /* to fit '\0' */;
}
static void update_environment(void) {
#ifdef DEBUG
    debug("update_environment()\t\t[%d]\n", getpid());
#endif

    /* If we haven't parsed the environment we also haven't modified it - so
     * nothing to do. */
    if (!initialized) {
        return;
    }

    char env[update_environment_buffer_size()];
    env[0] = 0;

    update_environment_buffer(env);

#if 0
    debug("    setenv('%s', '%s', 1)\n", ENV_NAME_FDS, env);
#endif

    setenv(ENV_NAME_FDS, env, 1 /* overwrite */);
}



static void tracked_fds_add(int fd) {
    if (fd < TRACKFDS_STATIC_COUNT) {
        tracked_fds[fd] = 1;
#if 0
        debug("tracked_fds_add(): %-3d\t\t[%d]\n", fd, getpid());
        tracked_fds_debug();
#endif
        return;
    }

    if (tracked_fds_list_count >= tracked_fds_list_space) {
        size_t new_space = tracked_fds_list_space + TRACKFDS_REALLOC_STEP;
        int *tmp = realloc(tracked_fds_list,
                           sizeof(*tracked_fds_list) * new_space);
        if (!tmp) {
            /* We can do nothing, just ignore the error. We made sure not to
             * destroy our state, so the new descriptor is ignored without any
             * other consequences. */
#ifdef DEBUG
            warning("realloc(tracked_fds_list, %zu) failed! [%d]\n",
                    sizeof(*tracked_fds_list) * new_space, getpid());
#endif
            return;
        }
        tracked_fds_list = tmp;
        tracked_fds_list_space = new_space;
    }

    tracked_fds_list[tracked_fds_list_count++] = fd;

#ifdef DEBUG
    debug("tracked_fds_add(): %-3d\t\t[%d]\n", fd, getpid());
    tracked_fds_debug();
#endif
}
static int tracked_fds_remove(int fd) {
    if (fd < TRACKFDS_STATIC_COUNT) {
        int old_value = tracked_fds[fd];
        tracked_fds[fd] = 0;

#if 0
        debug("tracked_fds_remove(): %-3d\t[%d]\n", fd, getpid());
        tracked_fds_debug();
#endif
        return old_value; /* Found vs. not found. */
    }

    size_t i;
    for (i = 0; i < tracked_fds_list_count; i++) {
        if (fd != tracked_fds_list[i]) {
            continue;
        }

        memmove(tracked_fds_list + i, tracked_fds_list + i + 1,
                sizeof(*tracked_fds_list) * (tracked_fds_list_count - i - 1));
        tracked_fds_list_count--;

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
    if (fd < TRACKFDS_STATIC_COUNT) {
        return tracked_fds[fd];
    }
    if (tracked_fds_list_count == 0) {
        return 0;
    }

    size_t i;
    for (i = 0; i < tracked_fds_list_count; i++) {
        if (fd == tracked_fds_list[i]) {
            return 1;
        }
    }
    return 0;
}

#endif

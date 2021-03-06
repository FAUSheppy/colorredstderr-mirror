/*
 * Utility functions to track file descriptors.
 *
 * Copyright (C) 2013-2018  Simon Ruderich
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

/* Check if filename occurs in the comma-separated list ignore. */
static int is_program_ignored(char const *filename, char const *ignore) {
    size_t length;
    size_t filename_length = strlen(filename);

#ifdef DEBUG
    debug("  is_program_ignored(\"%s\", \"%s\")\n", filename, ignore);
#endif

    for (; *ignore; ignore += length) {
        while (*ignore == ',') {
            ignore++;
        }

        length = strcspn(ignore, ",");
        if (length == 0) {
            break;
        }

        if (length != filename_length) {
            continue;
        }
        if (!strncmp(filename, ignore, length)) {
            return 1;
        }
    }

    return 0;
}

static int init_tracked_fds_list(size_t count) {
    assert(count > 0);

    /* Reduce reallocs. */
    count += TRACKFDS_REALLOC_STEP;

    tracked_fds_list = malloc(count * sizeof(*tracked_fds_list));
    if (!tracked_fds_list) {
#ifdef WARNING
        warning("malloc(tracked_fds_list, %d) failed [%d]\n",
                count * sizeof(*tracked_fds_list), getpid());
#endif
        return 0;
    }

    tracked_fds_list_space = count;
    return 1;
}

/*
 * Load tracked file descriptors from the environment. The environment is used
 * to pass the information to child processes.
 *
 * ENV_NAME_FDS and ENV_NAME_PRIVATE_FDS have the following format: Each
 * descriptor as string followed by a comma; there's a trailing comma.
 * Example: "2,4,".
 */
static void init_from_environment(void) {
#ifdef DEBUG
    debug("init_from_environment()\t\t[%d]\n", getpid());
#endif
    char const *env;

    int saved_errno = errno;

    assert(!initialized);

    initialized = 1;
    tracked_fds_list_count = 0;

    /* Don't color writes to stderr for this binary (and its children) if it's
     * contained in the comma-separated list in ENV_NAME_IGNORED_BINARIES. */
    env = getenv(ENV_NAME_IGNORED_BINARIES);
    if (env) {
        char path[512];

        /* TODO: Don't require /proc/. */
        ssize_t written = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (written > 0) {
            path[written] = 0; /* readlink() does not null-terminate! */
            if (is_program_ignored(path, env)) {
                return;
            }
        }
    }

    /* If ENV_NAME_FORCE_WRITE is set and not empty, allow writes to a non-tty
     * device. Use with care! Mainly used for the test suite. */
    env = getenv(ENV_NAME_FORCE_WRITE);
    if (env && env[0] != '\0') {
        force_write_to_non_tty = 1;
    }

    /* Prefer user defined list of file descriptors, fall back to file
     * descriptors passed through the environment from the parent process. */
    env = getenv(ENV_NAME_FDS);
    if (env) {
        used_fds_set_by_user = 1;
    } else {
        env = getenv(ENV_NAME_PRIVATE_FDS);
    }
    if (!env) {
        errno = saved_errno;
        return;
    }
#ifdef DEBUG
    debug("  getenv(\"%s\"): \"%s\"\n", ENV_NAME_FDS, env);
    debug("  getenv(\"%s\"): \"%s\"\n", ENV_NAME_PRIVATE_FDS, env);
#endif

    /* Environment must be treated read-only. */
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
            goto next;
        }

        /* Replace ',' to null-terminate number for atoi(). */
        *x = 0;

        int fd = atoi(last);
        if (fd < 0) {
            goto next;

        } else if (fd < TRACKFDS_STATIC_COUNT) {
            tracked_fds[fd] = 1;
        } else {
            if (!tracked_fds_list) {
                /* Pessimistic count estimate, but allocating a few more
                 * elements doesn't hurt. */
                if (!init_tracked_fds_list(count)) {
                    /* Couldn't allocate memory, skip this entry. */
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

    errno = saved_errno;
}

static char *update_environment_buffer_entry(char *x, int fd) {
    assert(fd >= 0);

    int length = snprintf(x, 10 + 1, "%d", fd);
    if (length >= 10 + 1 || length <= 0 /* shouldn't happen */) {
        /* Integer too big to fit the buffer, skip it. */
#ifdef WARNING
        warning("update_environment_buffer_entry(): truncated fd: %d [%d]\n",
                fd, getpid());
#endif
        return x;
    }

    /* Write comma after number. */
    x += length;
    *x++ = ',';
    /* Make sure the string is always null-terminated. */
    *x = 0;

    return x;
}
static void update_environment_buffer(char *x) {
    assert(initialized);

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
    assert(initialized);

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

    int saved_errno = errno;

    char env[update_environment_buffer_size()];
    env[0] = 0;

    update_environment_buffer(env);

#if 0
    debug("    setenv(\"%s\", \"%s\", 1)\n", ENV_NAME_PRIVATE_FDS, env);
#endif
    setenv(ENV_NAME_PRIVATE_FDS, env, 1 /* overwrite */);

    /* Child processes must use ENV_NAME_PRIVATE_FDS to get the updated list
     * of tracked file descriptors, not the static list provided by the user
     * in ENV_NAME_FDS.
     *
     * But only remove it if the static list in ENV_NAME_FDS was loaded by
     * init_from_environment() and merged into ENV_NAME_PRIVATE_FDS. */
    if (used_fds_set_by_user) {
        unsetenv(ENV_NAME_FDS);
    }

    errno = saved_errno;
}



static void tracked_fds_add(int fd) {
    assert(fd >= 0);

    if (fd < TRACKFDS_STATIC_COUNT) {
        tracked_fds[fd] = 1;
#if 0
        debug("tracked_fds_add(): %-3d\t\t[%d]\n", fd, getpid());
        tracked_fds_debug();
#endif
        return;
    }

    if (tracked_fds_list_count >= tracked_fds_list_space) {
        int saved_errno = errno;

        size_t new_space = tracked_fds_list_space + TRACKFDS_REALLOC_STEP;
        int *tmp = realloc(tracked_fds_list,
                           sizeof(*tracked_fds_list) * new_space);
        if (!tmp) {
            /* We can do nothing, just ignore the error. We made sure not to
             * destroy our state, so the new descriptor is ignored without any
             * other consequences. */
#ifdef WARNING
            warning("realloc(tracked_fds_list, %zu) failed! [%d]\n",
                    sizeof(*tracked_fds_list) * new_space, getpid());
#endif
            errno = saved_errno;
            return;
        }
        errno = saved_errno;

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
    assert(fd >= 0);

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

static int tracked_fds_find_slow(int fd) noinline;
/*
 * tracked_fds_find() is called for each hook call and should be as fast as
 * possible. As most file descriptors are < TRACKFDS_STATIC_COUNT, force the
 * compiler to inline that part which is almost exclusively used.
 *
 * Inlining tracked_fds_add()/tracked_fds_remove() isn't worth the effort as
 * they are not called often enough.
 */
inline static int tracked_fds_find(int fd) always_inline;
inline static int tracked_fds_find(int fd) {
    /* Invalid file descriptor. No assert() as we're called from the hooked
     * macro. */
    if (unlikely(fd < 0)) {
        return 0;
    }

    if (likely(fd < TRACKFDS_STATIC_COUNT)) {
        return tracked_fds[fd];
    }

    return tracked_fds_find_slow(fd);
}
static int tracked_fds_find_slow(int fd) {
    assert(initialized);
    assert(fd >= 0);

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

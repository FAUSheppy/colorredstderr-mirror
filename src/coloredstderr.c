/*
 * Hook output functions (like printf(3)) with LD_PRELOAD to color stderr (or
 * other file descriptors).
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

#include "config.h"

/* Must be loaded before the following headers. */
#include "ldpreload.h"

/* FIXME: use correct declaration for fcntl() */
#define fcntl fcntl_ignore

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#undef fcntl
/* Conflicting declaration in glibc. */
#undef fwrite_unlocked


/* Used by various functions, including debug(). */
static ssize_t (*real_write)(int, const void *, size_t);
static int (*real_close)(int);
static size_t (*real_fwrite)(const void *, size_t, size_t, FILE *);


#include "constants.h"
#ifdef DEBUG
# include "debug.h"
#endif

#include "macros.h"
#include "trackfds.h"



/* Should the "action" handler be invoked for this file descriptor? */
static int check_handle_fd(int fd) {
    /* Never touch anything not going to a terminal. */
    if (!isatty(fd)) {
        return 0;
    }

    /* Load state from environment. Only necessary once per process. */
    if (!tracked_fds) {
        init_from_environment();
    }

    if (tracked_fds_count == 0) {
        return 0;
    }
    return tracked_fds_find(fd);
}

static void dup_fd(int oldfd, int newfd) {
#ifdef DEBUG
    debug("%d -> %d\t\t\t[%d]\n", oldfd, newfd, getpid());
#endif

    if (!tracked_fds) {
        init_from_environment();
    }
    if (tracked_fds_count == 0) {
        return;
    }

    /* We are already tracking this file descriptor, add newfd to the list as
     * it will reference the same descriptor. */
    if (tracked_fds_find(oldfd)) {
        if (!tracked_fds_find(newfd)) {
            tracked_fds_add(newfd);
            update_environment();
        }
    /* We are not tracking this file descriptor, remove newfd from the list
     * (if present). */
    } else {
        if (tracked_fds_remove(newfd)) {
            update_environment();
        }
    }
}

static void close_fd(int fd) {
#ifdef DEBUG
    debug("%d -> .\t\t\t[%d]\n", fd, getpid());
#endif

    if (!tracked_fds) {
        init_from_environment();
    }

    if (tracked_fds_count == 0) {
        return;
    }
    tracked_fds_remove(fd);
}


/* "Action" handlers called when a file descriptor is matched. */

static char *pre_string;
static size_t pre_string_size;
static char *post_string;
static size_t post_string_size;

/* Load alternative pre/post strings from the environment if available, fall
 * back to default values. */
inline static void init_pre_post_string() {
    pre_string = getenv(ENV_NAME_PRE_STRING);
    if (!pre_string) {
        pre_string = DEFAULT_PRE_STRING;
    }
    pre_string_size = strlen(pre_string);

    post_string = getenv(ENV_NAME_POST_STRING);
    if (!post_string) {
        post_string = DEFAULT_POST_STRING;
    }
    post_string_size = strlen(post_string);
}

static void handle_fd_pre(int fd, int action) {
    (void)action;

    if (!pre_string || !post_string) {
        init_pre_post_string();
    }

    DLSYM_FUNCTION(real_write, "write");
    real_write(fd, pre_string, pre_string_size);
}
static void handle_fd_post(int fd, int action) {
    (void)action;

    /* write() already loaded above in handle_fd_pre(). */
    real_write(fd, post_string, post_string_size);
}

static void handle_file_pre(FILE *stream, int action) {
    (void)action;

    if (!pre_string || !post_string) {
        init_pre_post_string();
    }

    DLSYM_FUNCTION(real_fwrite, "fwrite");
    real_fwrite(pre_string, pre_string_size, 1, stream);
}
static void handle_file_post(FILE *stream, int action) {
    (void)action;

    /* fwrite() already loaded above in handle_file_pre(). */
    real_fwrite(post_string, post_string_size, 1, stream);
}



/* Hook all important output functions to manipulate their output. */

HOOK_FD3(ssize_t, write, fd,
         int, fd, const void *, buf, size_t, count)
HOOK_FILE4(size_t, fwrite, stream,
           const void *, ptr, size_t, size, size_t, nmemb, FILE *, stream)

/* puts(3) */
HOOK_FILE2(int, fputs, stream,
           const char *, s, FILE *, stream)
HOOK_FILE2(int, fputc, stream,
           int, c, FILE *, stream)
HOOK_FILE2(int, putc, stream,
           int, c, FILE *, stream)
HOOK_FILE1(int, putchar, stdout,
           int, c)
HOOK_FILE1(int, puts, stdout,
           const char *, s)

/* printf(3), excluding all s*() and vs*() functions (no output) */
HOOK_VAR_FILE1(int, printf, stdout, vprintf,
               const char *, format)
HOOK_VAR_FILE2(int, fprintf, stream, vfprintf,
               FILE *, stream, const char *, format)
HOOK_FILE2(int, vprintf, stdout,
           const char *, format, va_list, ap)
HOOK_FILE3(int, vfprintf, stream,
           FILE *, stream, const char *, format, va_list, ap)
/* Hardening functions (-D_FORTIFY_SOURCE=2). */
HOOK_VAR_FILE2(int, __printf_chk, stdout, __vprintf_chk,
               int, flag, const char *, format)
HOOK_VAR_FILE3(int, __fprintf_chk, fp, __vfprintf_chk,
               FILE *, fp, int, flag, const char *, format)
HOOK_FILE3(int, __vprintf_chk, stdout,
           int, flag, const char *, format, va_list, ap)
HOOK_FILE4(int, __vfprintf_chk, stream,
           FILE *, stream, int, flag, const char *, format, va_list, ap)

/* unlocked_stdio(3), only functions from above are hooked */
HOOK_FILE4(size_t, fwrite_unlocked, stream,
           const void *, ptr, size_t, size, size_t, nmemb, FILE *, stream)
HOOK_FILE2(int, fputs_unlocked, stream,
           const char *, s, FILE *, stream)
HOOK_FILE2(int, fputc_unlocked, stream,
           int, c, FILE *, stream)
HOOK_FILE2(int, putc_unlocked, stream,
           int, c, FILE *, stream)
HOOK_FILE1(int, putchar_unlocked, stdout,
           int, c)
HOOK_FILE1(int, puts_unlocked, stdout,
           const char *, s)


/* Hook functions which duplicate file descriptors to track them. */

static int (*real_dup)(int);
static int (*real_dup2)(int, int);
static int (*real_dup3)(int, int, int);
int dup(int oldfd) {
    int newfd;

    DLSYM_FUNCTION(real_dup, "dup");

    newfd = real_dup(oldfd);
    if (newfd != -1) {
        int saved_errno = errno;
        dup_fd(oldfd, newfd);
        errno = saved_errno;
    }

    return newfd;
}
int dup2(int oldfd, int newfd) {
    DLSYM_FUNCTION(real_dup2, "dup2");

    newfd = real_dup2(oldfd, newfd);
    if (newfd != -1) {
        int saved_errno = errno;
        dup_fd(oldfd, newfd);
        errno = saved_errno;
    }

    return newfd;
}
int dup3(int oldfd, int newfd, int flags) {
    DLSYM_FUNCTION(real_dup3, "dup3");

    newfd = real_dup3(oldfd, newfd, flags);
    if (newfd != -1) {
        int saved_errno = errno;
        dup_fd(oldfd, newfd);
        errno = saved_errno;
    }

    return newfd;
}

static int (*real_fcntl)(int, int, int);
int fcntl(int fd, int cmd, int arg) {
    int result;

    DLSYM_FUNCTION(real_fcntl, "fcntl");

    result = real_fcntl(fd, cmd, arg);
    /* We only care about duping fds. */
    if (cmd == F_DUPFD) {
        int saved_errno = errno;
        dup_fd(fd, result);
        errno = saved_errno;
    }

    return result;
}

static int (*real_close)(int);
int close(int fd) {
    DLSYM_FUNCTION(real_close, "close");

    close_fd(fd);
    return real_close(fd);
}
static int (*real_fclose)(FILE *);
int fclose(FILE *fp) {
    DLSYM_FUNCTION(real_fclose, "fclose");

    close_fd(fileno(fp));
    return real_fclose(fp);
}

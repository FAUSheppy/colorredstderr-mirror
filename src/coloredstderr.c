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

#include <config.h>

/* Must be loaded before the following headers. */
#include "ldpreload.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_ERROR_H
# include <error.h>
#endif

/* Conflicting declaration in glibc. */
#undef fwrite_unlocked


/* Used by various functions, including debug(). */
static ssize_t (*real_write)(int, void const *, size_t);
static int (*real_close)(int);
static size_t (*real_fwrite)(void const *, size_t, size_t, FILE *);

/* Did we already (try to) parse the environment and setup the necessary
 * variables? */
static int initialized;
/* Force hooked writes even when not writing to a tty. Used for tests. */
static int force_write_to_non_tty;


#include "constants.h"
#ifdef DEBUG
# include "debug.h"
#endif

#include "hookmacros.h"
#include "trackfds.h"



/* Should the "action" handler be invoked for this file descriptor? */
static int check_handle_fd(int fd) {
    /* Load state from environment. Only necessary once per process. */
    if (!initialized) {
        init_from_environment();
    }

    /* tracked_fds_find() is most likely faster than calling isatty(),
     * therefore check if we are tracking this file descriptor first. */
    if (!tracked_fds_find(fd)) {
        return 0;
    }

    /* Never touch anything not going to a terminal - unless we are explicitly
     * asked to do so. */
    if (force_write_to_non_tty) {
        return 1;
    }

    int saved_errno = errno;
    int result = isatty(fd);
    errno = saved_errno;

    return result;
}

static void dup_fd(int oldfd, int newfd) {
#ifdef DEBUG
    debug("%3d -> %3d\t\t\t[%d]\n", oldfd, newfd, getpid());
#endif

    if (!initialized) {
        init_from_environment();
    }

    /* We are already tracking this file descriptor, add newfd to the list as
     * it will reference the same descriptor. */
    if (tracked_fds_find(oldfd)) {
        if (!tracked_fds_find(newfd)) {
            tracked_fds_add(newfd);
        }
    /* We are not tracking this file descriptor, remove newfd from the list
     * (if present). */
    } else {
        tracked_fds_remove(newfd);
    }
}

static void close_fd(int fd) {
#ifdef DEBUG
    debug("%3d ->   .\t\t\t[%d]\n", fd, getpid());
#endif

    if (!initialized) {
        init_from_environment();
    }

    tracked_fds_remove(fd);
}


/* "Action" handlers called when a file descriptor is matched. */

static char const *pre_string;
static size_t pre_string_size;
static char const *post_string;
static size_t post_string_size;

/* Load alternative pre/post strings from the environment if available, fall
 * back to default values. */
inline static void init_pre_post_string(void) {
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

static void handle_fd_pre(int fd) {
    int saved_errno = errno;

    if (!pre_string || !post_string) {
        init_pre_post_string();
    }

    DLSYM_FUNCTION(real_write, "write");
    real_write(fd, pre_string, pre_string_size);

    errno = saved_errno;
}
static void handle_fd_post(int fd) {
    int saved_errno = errno;

    /* write() already loaded above in handle_fd_pre(). */
    real_write(fd, post_string, post_string_size);

    errno = saved_errno;
}

static void handle_file_pre(FILE *stream) {
    int saved_errno = errno;

    if (!pre_string || !post_string) {
        init_pre_post_string();
    }

    DLSYM_FUNCTION(real_fwrite, "fwrite");
    real_fwrite(pre_string, pre_string_size, 1, stream);

    errno = saved_errno;
}
static void handle_file_post(FILE *stream) {
    int saved_errno = errno;

    /* fwrite() already loaded above in handle_file_pre(). */
    real_fwrite(post_string, post_string_size, 1, stream);

    errno = saved_errno;
}



/* Hook all important output functions to manipulate their output. */

HOOK_FD3(ssize_t, write, fd,
         int, fd, void const *, buf, size_t, count)
HOOK_FILE4(size_t, fwrite, stream,
           void const *, ptr, size_t, size, size_t, nmemb, FILE *, stream)

/* puts(3) */
HOOK_FILE2(int, fputs, stream,
           char const *, s, FILE *, stream)
HOOK_FILE2(int, fputc, stream,
           int, c, FILE *, stream)
HOOK_FILE2(int, putc, stream,
           int, c, FILE *, stream)
HOOK_FILE1(int, putchar, stdout,
           int, c)
HOOK_FILE1(int, puts, stdout,
           char const *, s)

/* printf(3), excluding all s*() and vs*() functions (no output) */
HOOK_VAR_FILE1(int, printf, stdout, vprintf,
               char const *, format)
HOOK_VAR_FILE2(int, fprintf, stream, vfprintf,
               FILE *, stream, char const *, format)
HOOK_FILE2(int, vprintf, stdout,
           char const *, format, va_list, ap)
HOOK_FILE3(int, vfprintf, stream,
           FILE *, stream, char const *, format, va_list, ap)
/* Hardening functions (-D_FORTIFY_SOURCE=2), only functions from above */
HOOK_VAR_FILE2(int, __printf_chk, stdout, __vprintf_chk,
               int, flag, char const *, format)
HOOK_VAR_FILE3(int, __fprintf_chk, fp, __vfprintf_chk,
               FILE *, fp, int, flag, char const *, format)
HOOK_FILE3(int, __vprintf_chk, stdout,
           int, flag, char const *, format, va_list, ap)
HOOK_FILE4(int, __vfprintf_chk, stream,
           FILE *, stream, int, flag, char const *, format, va_list, ap)

/* unlocked_stdio(3), only functions from above are hooked */
HOOK_FILE4(size_t, fwrite_unlocked, stream,
           void const *, ptr, size_t, size, size_t, nmemb, FILE *, stream)
HOOK_FILE2(int, fputs_unlocked, stream,
           char const *, s, FILE *, stream)
HOOK_FILE2(int, fputc_unlocked, stream,
           int, c, FILE *, stream)
HOOK_FILE2(int, putc_unlocked, stream,
           int, c, FILE *, stream)
HOOK_FILE1(int, putchar_unlocked, stdout,
           int, c)

/* perror(3) */
HOOK_VOID1(void, perror, STDERR_FILENO,
           char const *, s)

/* error(3) */
#ifdef HAVE_ERROR_H
static void  error_vararg(int status, int errnum,
                   char const *filename, unsigned int linenum,
                   char const *format, va_list ap) {
    static char const *last_filename;
    static unsigned int last_linenum;

    /* Skip this error message if requested and if there was already an error
     * in the same file/line. */
    if (error_one_per_line
            && filename != NULL && linenum != 0
            && filename == last_filename && linenum == last_linenum) {
        return;
    }
    last_filename = filename;
    last_linenum  = linenum;

    error_message_count++;

    fflush(stdout);

    if (error_print_progname) {
        error_print_progname();
    } else {
        fprintf(stderr, "%s:", program_invocation_name);
    }
    if (filename != NULL && linenum != 0) {
        fprintf(stderr, "%s:%u:", filename, linenum);
        if (error_print_progname) {
            fprintf(stderr, " ");
        }
    }
    if (!error_print_progname) {
        fprintf(stderr, " ");
    }


    vfprintf(stderr, format, ap);

    if (errnum != 0) {
        fprintf(stderr, ": %s", strerror(errnum));
    }

    fprintf(stderr, "\n");

    if (status != 0) {
        exit(status);
    }
}
void error_at_line(int status, int errnum,
                   char const *filename, unsigned int linenum,
                   char const *format, ...) {
    va_list ap;

    va_start(ap, format);
    error_vararg(status, errnum, filename, linenum, format, ap);
    va_end(ap);
}
void error(int status, int errnum, char const *format, ...) {
    va_list ap;

    va_start(ap, format);
    error_vararg(status, errnum, NULL, 0, format, ap);
    va_end(ap);
}
#endif


/* Hook functions which duplicate file descriptors to track them. */

static int (*real_dup)(int);
static int (*real_dup2)(int, int);
static int (*real_dup3)(int, int, int);
int dup(int oldfd) {
    int newfd;

    DLSYM_FUNCTION(real_dup, "dup");

    newfd = real_dup(oldfd);
    if (newfd != -1) {
        dup_fd(oldfd, newfd);
    }

    return newfd;
}
int dup2(int oldfd, int newfd) {
    DLSYM_FUNCTION(real_dup2, "dup2");

    newfd = real_dup2(oldfd, newfd);
    if (newfd != -1) {
        dup_fd(oldfd, newfd);
    }

    return newfd;
}
int dup3(int oldfd, int newfd, int flags) {
    DLSYM_FUNCTION(real_dup3, "dup3");

    newfd = real_dup3(oldfd, newfd, flags);
    if (newfd != -1) {
        dup_fd(oldfd, newfd);
    }

    return newfd;
}

static int (*real_fcntl)(int, int, ...);
int fcntl(int fd, int cmd, ...) {
    int result;
    va_list ap;

    DLSYM_FUNCTION(real_fcntl, "fcntl");

    /* fcntl() takes different types of arguments depending on the cmd type
     * (int, void and pointers are used at the moment). Handling these
     * arguments for different systems and with possible changes in the future
     * is error prone.
     *
     * Therefore always retrieve a void-pointer from our arguments (even if it
     * wasn't there) and pass it to real_fcntl(). This shouldn't cause any
     * problems because a void-pointer is most-likely bigger than an int
     * (something which is not true in reverse) and shouldn't cause
     * truncation. For register based calling conventions an invalid register
     * content is passed, but ignored by real_fcntl(). Not perfect, but should
     * work fine.
     */
    va_start(ap, cmd);
    result = real_fcntl(fd, cmd, va_arg(ap, void *));
    va_end(ap);

    /* We only care about duping fds. */
    if (cmd == F_DUPFD && result != -1) {
        dup_fd(fd, result);
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


/* Hook functions which are necessary for correct tracking. */

#if defined(HAVE_VFORK) && defined(HAVE_FORK)
pid_t vfork(void) {
    /* vfork() is similar to fork() but the address space is shared between
     * father and child. It's designed for fork()/exec() usage because it's
     * faster than fork(). However according to the POSIX standard the "child"
     * isn't allowed to perform any memory-modifications before the exec()
     * (except the pid_t result variable of vfork()).
     *
     * As some programs don't adhere to the standard (e.g. the "child" closes
     * or dups a descriptor before the exec()) and this breaks our tracking of
     * file descriptors (e.g. it gets closed in the parent as well), we just
     * fork() instead. This is in compliance with the POSIX standard and as
     * most systems use copy-on-write anyway not a performance issue. */
    return fork();
}
#endif


/* Hook execve() and the other exec*() functions. Some shells use exec*() with
 * a custom environment which doesn't necessarily contain our updates to
 * ENV_NAME_FDS. It's also faster to update the environment only when
 * necessary, right before the exec() to pass it to the new process. */

static int (*real_execve)(char const *filename, char * const argv[], char * const env[]);
int execve(char const *filename, char * const argv[], char * const env[]) {
    DLSYM_FUNCTION(real_execve, "execve");

    int found = 0;
    size_t index = 0;

    /* Count arguments and search for existing ENV_NAME_FDS environment
     * variable. */
    size_t count = 0;
    char * const *x = env;
    while (*x) {
        if (!strncmp(*x, ENV_NAME_FDS "=", strlen(ENV_NAME_FDS) + 1)) {
            found = 1;
            index = count;
        }

        x++;
        count++;
    }
    /* Terminating NULL. */
    count++;

    char *env_copy[count + 1 /* space for our new entry if necessary */];
    memcpy(env_copy, env, count * sizeof(char *));

    /* Make sure the information from the environment is loaded. We can't just
     * do nothing (like update_environment()) because the caller might pass a
     * different environment which doesn't include any of our settings. */
    if (!initialized) {
        init_from_environment();
    }

    char fds_env[strlen(ENV_NAME_FDS) + 1 + update_environment_buffer_size()];
    strcpy(fds_env, ENV_NAME_FDS "=");
    update_environment_buffer(fds_env + strlen(ENV_NAME_FDS) + 1);

    if (found) {
        env_copy[index] = fds_env;
    } else {
        /* If the process removed ENV_NAME_FDS from the environment, re-add
         * it. */
        env_copy[count-1] = fds_env;
        env_copy[count] = NULL;
    }

    return real_execve(filename, argv, env_copy);
}

#define EXECL_COPY_VARARGS_START(args) \
    va_list ap; \
    char *x; \
    \
    /* Count arguments. */ \
    size_t count = 1; /* arg */ \
    va_start(ap, arg); \
    while (va_arg(ap, char const *)) { \
        count++; \
    } \
    va_end(ap); \
    \
    /* Copy varargs. */ \
    char *args[count + 1 /* terminating NULL */]; \
    args[0] = (char *)arg; \
    \
    size_t i = 1; \
    va_start(ap, arg); \
    while ((x = va_arg(ap, char *))) { \
        args[i++] = x; \
    } \
    args[i] = NULL;
#define EXECL_COPY_VARARGS_END(args) \
    va_end(ap);
#define EXECL_COPY_VARARGS(args) \
    EXECL_COPY_VARARGS_START(args); \
    EXECL_COPY_VARARGS_END(args);

int execl(char const *path, char const *arg, ...) {
    EXECL_COPY_VARARGS(args);

    update_environment();
    return execv(path, args);
}

int execlp(char const *file, char const *arg, ...) {
    EXECL_COPY_VARARGS(args);

    update_environment();
    return execvp(file, args);
}

int execle(char const *path, char const *arg, ... /*, char * const envp[] */) {
    EXECL_COPY_VARARGS_START(args);
    /* Get envp[] located after arguments. */
    char * const *envp = va_arg(ap, char * const *);
    EXECL_COPY_VARARGS_END(args);

    return execve(path, args, envp);
}

static int (*real_execv)(char const *path, char * const argv[]);
int execv(char const *path, char * const argv[]) {
    DLSYM_FUNCTION(real_execv, "execv");

    update_environment();
    return real_execv(path, argv);
}

static int (*real_execvp)(char const *path, char * const argv[]);
int execvp(char const *path, char * const argv[]) {
    DLSYM_FUNCTION(real_execvp, "execvp");

    update_environment();
    return real_execvp(path, argv);
}

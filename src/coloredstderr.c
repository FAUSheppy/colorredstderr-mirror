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

#include "compiler.h"

/* Must be loaded before the following headers. */
#include "ldpreload.h"

/* Disable assert()s if not compiled with --enable-debug. */
#ifndef DEBUG
# define NDEBUG
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_ERR_H
# include <err.h>
#endif
#ifdef HAVE_ERROR_H
# include <error.h>
#endif
#ifdef HAVE_STRUCT__IO_FILE__FILENO
# include <libio.h>
#endif

/* The following functions may be macros. Undefine them or they cause build
 * failures when used in our hook macros below. */

/* In glibc, real fwrite_unlocked() is called in macro. */
#ifdef HAVE_FWRITE_UNLOCKED
# undef fwrite_unlocked
#endif
/* In Clang when compiling with hardening flags (fortify) on Debian Wheezy. */
#undef printf
#undef fprintf
/* On FreeBSD (9.1), __swbuf() is used instead of these macros. */
#ifdef HAVE___SWBUF
# undef putc
# undef putc_unlocked
# undef putchar
# undef putchar_unlocked
#endif


/* Used by various functions, including debug(). */
static ssize_t (*real_write)(int, void const *, size_t);
static int (*real_close)(int);
static size_t (*real_fwrite)(void const *, size_t, size_t, FILE *);

/* Did we already (try to) parse the environment and setup the necessary
 * variables? */
static int initialized;
/* Force hooked writes even when not writing to a tty. Used for tests. */
static int force_write_to_non_tty;
/* Was ENV_NAME_FDS found and used when init_from_environment() was called?
 * This is not true if the process set it manually after initialization. */
static int used_fds_set_by_user;
/* Was any of our handle_*_pre()/handle_*_post() functions called recursively?
 * If so don't print the pre/post string for the recursive calls. This is
 * necessary on some systems (e.g. FreeBSD 9.1) which call multiple hooked
 * functions while printing a string (e.g. a FILE * and a fd hook function is
 * called). */
static int handle_recursive;


#include "constants.h"
#ifdef WARNING
# include "debug.h"
#endif

#include "hookmacros.h"
#include "trackfds.h"



/* See hookmacros.h for the decision if a function call is colored. */


/* Prevent inlining into hook functions because it may increase the number of
 * spilled registers unnecessarily. As it's not called very often accept the
 * additional call. */
static int isatty_noinline(int fd) noinline;
static int isatty_noinline(int fd) {
    assert(fd >= 0);

    int saved_errno = errno;
    int result = isatty(fd);
    errno = saved_errno;

    return result;
}


static void dup_fd(int oldfd, int newfd) {
#ifdef DEBUG
    debug("%3d -> %3d\t\t\t[%d]\n", oldfd, newfd, getpid());
#endif

    assert(oldfd >= 0 && newfd >= 0);

    if (unlikely(!initialized)) {
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

    assert(fd >= 0);

    if (unlikely(!initialized)) {
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
static void init_pre_post_string(void) {
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

/* Don't inline any of the pre/post functions. Keep the hook function as small
 * as possible for speed reasons. */
static void handle_fd_pre(int fd) noinline;
static void handle_fd_post(int fd) noinline;
static void handle_file_pre(FILE *stream) noinline;
static void handle_file_post(FILE *stream) noinline;

static void handle_fd_pre(int fd) {
    if (handle_recursive++ > 0) {
        return;
    }

    int saved_errno = errno;

    if (unlikely(!pre_string)) {
        init_pre_post_string();
    }

    DLSYM_FUNCTION(real_write, "write");
    real_write(fd, pre_string, pre_string_size);

    errno = saved_errno;
}
static void handle_fd_post(int fd) {
    if (--handle_recursive > 0) {
        return;
    }

    int saved_errno = errno;

    /* write() already loaded above in handle_fd_pre(). */
    real_write(fd, post_string, post_string_size);

    errno = saved_errno;
}

static void handle_file_pre(FILE *stream) {
    if (handle_recursive++ > 0) {
        return;
    }

    int saved_errno = errno;

    if (unlikely(!pre_string)) {
        init_pre_post_string();
    }

    DLSYM_FUNCTION(real_fwrite, "fwrite");
    real_fwrite(pre_string, pre_string_size, 1, stream);

    errno = saved_errno;
}
static void handle_file_post(FILE *stream) {
    if (--handle_recursive > 0) {
        return;
    }

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
/* The glibc uses a macro for putc() which expands to _IO_putc(). However
 * sometimes the raw putc() is used as well, not sure why. Make sure to hook
 * it too. */
#ifdef putc
# undef putc
HOOK_FILE2(int, putc, stream,
           int, c, FILE *, stream)
#endif
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
#ifdef HAVE_FWRITE_UNLOCKED
HOOK_FILE4(size_t, fwrite_unlocked, stream,
           void const *, ptr, size_t, size, size_t, nmemb, FILE *, stream)
#endif
#ifdef HAVE_FPUTS_UNLOCKED
HOOK_FILE2(int, fputs_unlocked, stream,
           char const *, s, FILE *, stream)
#endif
#ifdef HAVE_FPUTC_UNLOCKED
HOOK_FILE2(int, fputc_unlocked, stream,
           int, c, FILE *, stream)
#endif
HOOK_FILE2(int, putc_unlocked, stream,
           int, c, FILE *, stream)
HOOK_FILE1(int, putchar_unlocked, stdout,
           int, c)
/* glibc defines (_IO_)putc_unlocked() to a macro which either updates the
 * output buffer or calls __overflow(). As this code is inlined we can't
 * handle the first case, but if __overflow() is called we can color that
 * part. As writes to stderr are never buffered, __overflow() is always called
 * and everything works fine. This is only a problem if stdout is dupped to
 * stderr (which shouldn't be the case too often). */
#if defined(HAVE_STRUCT__IO_FILE__FILENO) && defined(HAVE___OVERFLOW)
/* _IO_FILE is glibc's representation of FILE. */
HOOK_FILE2(int, __overflow, f, _IO_FILE *, f, int, ch)
#endif
/* Same for FreeBSD's libc. However it's more aggressive: The inline writing
 * and __swbuf() are also used for normal output (e.g. putc()). Writing to
 * stderr is still fine; it always calls __swbuf() as stderr is always
 * unbufferd. */
#ifdef HAVE___SWBUF
HOOK_FILE2(int, __swbuf, f, int, c, FILE *, f)
#endif

/* perror(3) */
HOOK_VOID1(void, perror, STDERR_FILENO,
           char const *, s)

/* err(3), non standard BSD extension */
#ifdef HAVE_ERR_H
HOOK_VAR_VOID2(void, err, STDERR_FILENO, verr,
               int, eval, char const *, fmt)
HOOK_VAR_VOID2(void, errx, STDERR_FILENO, verrx,
               int, eval, char const *, fmt)
HOOK_VAR_VOID1(void, warn, STDERR_FILENO, vwarn,
               char const *, fmt)
HOOK_VAR_VOID1(void, warnx, STDERR_FILENO, vwarnx,
               char const *, fmt)
HOOK_FUNC_SIMPLE3(void, verr, int, eval, const char *, fmt, va_list, args) {
    /* Can't use verr() directly as it terminates the process which prevents
     * the post string from being printed. */
    vwarn(fmt, args);
    exit(eval);
}
HOOK_FUNC_SIMPLE3(void, verrx, int, eval, const char *, fmt, va_list, args) {
    /* See verr(). */
    vwarnx(fmt, args);
    exit(eval);
}
HOOK_VOID2(void, vwarn, STDERR_FILENO,
           char const *, fmt, va_list, args)
HOOK_VOID2(void, vwarnx, STDERR_FILENO,
           char const *, fmt, va_list, args)
#endif

/* error(3), non-standard GNU extension */
#ifdef HAVE_ERROR_H
static void error_vararg(int status, int errnum,
                         char const *filename, unsigned int linenum,
                         char const *format, va_list ap) {
    static char const *last_filename;
    static unsigned int last_linenum;

    /* Skip this error message if requested and if there was already an error
     * in the same file/line. */
    if (error_one_per_line
            && filename != NULL && linenum != 0
            && filename == last_filename && linenum == last_linenum) {
        goto out;
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

out:
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

/* int dup(int) */
HOOK_FUNC_DEF1(int, dup, int, oldfd) {
    int newfd;

    DLSYM_FUNCTION(real_dup, "dup");

    newfd = real_dup(oldfd);
    if (newfd > -1) {
        dup_fd(oldfd, newfd);
    }

    return newfd;
}
/* int dup2(int, int) */
HOOK_FUNC_DEF2(int, dup2, int, oldfd, int, newfd) {
    DLSYM_FUNCTION(real_dup2, "dup2");

    newfd = real_dup2(oldfd, newfd);
    if (newfd > -1) {
        dup_fd(oldfd, newfd);
    }

    return newfd;
}
/* int dup3(int, int, int) */
HOOK_FUNC_DEF3(int, dup3, int, oldfd, int, newfd, int, flags) {
    DLSYM_FUNCTION(real_dup3, "dup3");

    newfd = real_dup3(oldfd, newfd, flags);
    if (newfd > -1) {
        dup_fd(oldfd, newfd);
    }

    return newfd;
}

/* int fcntl(int, int, ...) */
HOOK_FUNC_VAR_DEF2(int, fcntl, int, fd, int, cmd /*, ... */) {
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
    if (cmd == F_DUPFD && result > -1) {
        dup_fd(fd, result);
    }

    return result;
}

/* int close(int) */
HOOK_FUNC_DEF1(int, close, int, fd) {
    DLSYM_FUNCTION(real_close, "close");

    if (fd >= 0) {
        close_fd(fd);
    }
    return real_close(fd);
}
/* int fclose(FILE *) */
HOOK_FUNC_DEF1(int, fclose, FILE *, fp) {
    int fd;

    DLSYM_FUNCTION(real_fclose, "fclose");

    if (fp != NULL && (fd = fileno(fp)) >= 0) {
        close_fd(fd);
    }
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
 * ENV_NAME_PRIVATE_FDS. It's also faster to update the environment only when
 * necessary, right before the exec(), to pass it to the new program. */

/* int execve(char const *, char * const [], char * const []) */
HOOK_FUNC_DEF3(int, execve, char const *, filename, char * const *, argv, char * const *, env) {
    DLSYM_FUNCTION(real_execve, "execve");

    /* Count environment variables. */
    size_t count = 0;
    char * const *x = env;
    while (*x++) {
        count++;
    }
    /* Terminating NULL. */
    count++;

    char *env_copy[count + 1 /* space for our new entry if necessary */];

    /* Make sure the information from the environment is loaded. We can't just
     * do nothing (like update_environment()) because the caller might pass a
     * different environment which doesn't include any of our settings. */
    if (!initialized) {
        init_from_environment();
    }

    char fds_env[strlen(ENV_NAME_PRIVATE_FDS)
                 + 1 + update_environment_buffer_size()];
    strcpy(fds_env, ENV_NAME_PRIVATE_FDS "=");
    update_environment_buffer(fds_env + strlen(ENV_NAME_PRIVATE_FDS) + 1);

    int found = 0;
    char **x_copy = env_copy;

    /* Copy the environment manually; allows skipping elements. */
    x = env;
    while ((*x_copy = *x)) {
        /* Remove ENV_NAME_FDS if we've already used its value. The new
         * program must use the updated list from ENV_NAME_PRIVATE_FDS. */
        if (used_fds_set_by_user
                && !strncmp(*x, ENV_NAME_FDS "=", strlen(ENV_NAME_FDS) + 1)) {
            x++;
            continue;
        /* Update ENV_NAME_PRIVATE_FDS. */
        } else if (!strncmp(*x, ENV_NAME_PRIVATE_FDS "=",
                            strlen(ENV_NAME_PRIVATE_FDS) + 1)) {
            *x_copy = fds_env;
            found = 1;
        }

        x++;
        x_copy++;
    }
    /* The loop "condition" NULL-terminates env_copy. */

    if (!found) {
        /* If the process removed ENV_NAME_PRIVATE_FDS from the environment,
         * re-add it. */
        *x_copy++ = fds_env;
        *x_copy++ = NULL;
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
    while (va_arg(ap, char *)) { \
        count++; \
    } \
    va_end(ap); \
    \
    /* Copy varargs. */ \
    char *args[count + 1 /* terminating NULL */]; \
    args[0] = (char *)arg; /* there's no other way around the cast */ \
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

    /* execv() updates the environment. */
    return execv(path, args);
}
int execlp(char const *file, char const *arg, ...) {
    EXECL_COPY_VARARGS(args);

    /* execvp() updates the environment. */
    return execvp(file, args);
}
int execle(char const *path, char const *arg, ... /*, char * const envp[] */) {
    char * const *envp;

    EXECL_COPY_VARARGS_START(args);
    /* Get envp[] located after arguments. */
    envp = va_arg(ap, char * const *);
    EXECL_COPY_VARARGS_END(args);

    /* execve() updates the environment. */
    return execve(path, args, envp);
}

/* int execv(char const *, char * const []) */
HOOK_FUNC_DEF2(int, execv, char const *, path, char * const *, argv) {
    DLSYM_FUNCTION(real_execv, "execv");

    update_environment();
    return real_execv(path, argv);
}

/* int execvp(char const *, char * const []) */
HOOK_FUNC_DEF2(int, execvp, char const *, file, char * const *, argv) {
    DLSYM_FUNCTION(real_execvp, "execvp");

    update_environment();
    return real_execvp(file, argv);
}

#ifdef HAVE_EXECVPE
extern char **environ;
int execvpe(char const *file, char * const argv[], char * const envp[]) {
    int result;
    char **old_environ = environ;

    /* Fake the environment so we can reuse execvp(). */
    environ = (char **)envp;

    /* execvp() updates the environment. */
    result = execvp(file, argv);

    environ = old_environ;
    return result;
}
#endif

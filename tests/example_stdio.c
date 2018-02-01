/*
 * Test all hooked stdio.h functions.
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

#include <config.h>

/* For {fwrite,fputs,fputc}_unlocked(), if available. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "example.h"
#include "../src/compiler.h"


/* These are not in POSIX. */
#ifndef HAVE_FWRITE_UNLOCKED
static size_t fwrite_unlocked(void const *ptr, size_t size, size_t n, FILE *stream) {
    return fwrite(ptr, size, n, stream);
}
#endif
#ifndef HAVE_FPUTS_UNLOCKED
static int fputs_unlocked(char const *s, FILE *stream) {
    return fputs(s, stream);
}
#endif
#ifndef HAVE_FPUTC_UNLOCKED
static int fputc_unlocked(int c, FILE *stream) {
    return fputc(c, stream);
}
#endif


static int out;

static void test_vprintf(char const *format, ...) noinline;
static void test_vfprintf(FILE *stream, char const *format, ...) noinline;
static void NEWLINE(void) noinline;

static void test_vprintf(char const *format, ...) {
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
static void test_vfprintf(FILE *stream, char const *format, ...) {
    va_list ap;

    va_start(ap, format);
    vfprintf(stream, format, ap);
    va_end(ap);
}

static void NEWLINE(void) {
    fflush(stdout);
    fflush(stderr);
    xwrite(out, "\n", 1);
}


int main(int argc, char **argv unused) {
    /* stdout */

    out = dup(STDOUT_FILENO);
    if (out == -1) {
        perror("dup");
        return EXIT_FAILURE;
    }

    /* Redirect stdout to stderr so we can test all functions, including those
     * not writing to stderr. */
    xdup2(STDERR_FILENO, STDOUT_FILENO);

    /* stdout redirected to stderr. */

    xwrite(STDOUT_FILENO, "write()", 7); NEWLINE();
    fwrite("fwrite()", 8, 1, stdout);   NEWLINE();

    /* puts(3) */
    fputs("fputs()", stdout); NEWLINE();
    fputc('a', stdout);       NEWLINE();
    putc('b', stdout);        NEWLINE();
    putchar('c');             NEWLINE();
    puts("puts()");           NEWLINE();

    /* printf(3) */
    printf("%s [%d]", "printf()", argc);                  NEWLINE();
    fprintf(stdout, "%s [%d]", "fprintf()", argc);        NEWLINE();
    test_vprintf("%s [%d]", "vprintf()", argc);           NEWLINE();
    test_vfprintf(stdout, "%s [%d]", "vfprintf()", argc); NEWLINE();

    /* unlocked_stdio(3) */
    fwrite_unlocked("fwrite_unlocked()", 17, 1, stdout); NEWLINE();
    fputs_unlocked("fputs_unlocked()", stdout);          NEWLINE();
    /* FIXME: 'x', 'y' and 'z' are not colored */
    fputc_unlocked('x', stdout);                         NEWLINE();
    putc_unlocked('y', stdout);                          NEWLINE();
    putchar_unlocked('z');                               NEWLINE();


    /* stderr */

    xwrite(STDERR_FILENO, "write()", 7); NEWLINE();
    fwrite("fwrite()", 8, 1, stderr);   NEWLINE();

    /* puts(3) */
    fputs("fputs()", stderr); NEWLINE();
    fputc('a', stderr);       NEWLINE();
    putc('b', stderr);        NEWLINE();

    /* printf(3) */
    fprintf(stderr, "%s [%d]", "fprintf()", argc);        NEWLINE();
    test_vfprintf(stderr, "%s [%d]", "vfprintf()", argc); NEWLINE();

    /* unlocked_stdio(3) */
    fwrite_unlocked("fwrite_unlocked()", 17, 1, stderr); NEWLINE();
    fputs_unlocked("fputs_unlocked()", stderr);          NEWLINE();
    fputc_unlocked('x', stderr);                         NEWLINE();
    putc_unlocked('y', stderr);                          NEWLINE();

    return EXIT_SUCCESS;
}

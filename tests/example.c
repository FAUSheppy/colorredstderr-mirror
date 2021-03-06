/*
 * Test basic features of coloredstderr.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "../src/compiler.h"
#include "example.h"


int main(int argc, char **argv unused) {
    fprintf(stderr, "write to stderr: %d\n", argc);
    printf("write to stdout\n");
    fflush(stdout);

    errno = ENOMEM;
    perror("error!");

    xwrite(STDERR_FILENO, "write to stderr 2", 17);
    xwrite(STDOUT_FILENO, "write to stdout 2", 17);

    fprintf(stderr, "\n");
    fprintf(stdout, "\n");
    fflush(stdout);

    /* Check usage of tracked_fds_list (at least in parts). */
    xdup2(STDERR_FILENO, 471);
    xdup2(471, 42);
    xwrite(471, "more on stderr\n", 15);
    close(471);
    xdup2(STDOUT_FILENO, 471);
    xwrite(42, "stderr ...\n", 11);
    xwrite(471, "more on stdout\n", 15);

    /* Glibc uses __overflow() for this ... */
    putc_unlocked('x', stderr);
    putc_unlocked('\n', stdout);

    /* Test invalid stuff. */
    write(-3, "foo", 3);
    close(-42);
    close(-4711);
    /* Can't test this, results in a segfault with the "normal" fclose(). */
    /*fclose(NULL);*/
    dup(-12);
    dup2(12, -42);

    return EXIT_SUCCESS;
}

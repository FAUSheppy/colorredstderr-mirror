/*
 * Test error() and error_at_line(). Non-standard, GNU only.
 *
 * Copyright (C) 2013-2014  Simon Ruderich
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

#define _GNU_SOURCE /* for program_invocation_name */
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../src/compiler.h"
#include "example.h"


void (*error_print_progname)(void);


static void print_progname(void) {
    fprintf(stderr, "PROG");
}


int main(int argc unused, char **argv unused) {
    pid_t pid;

    char name[] = "./example_error";
    program_invocation_name = name;

    error(0, 0, "<message>");
    error_at_line(0, 0, "file", 42, "<message>");
    FORKED_TEST(pid) { error(1, 0, "<message>"); }
    FORKED_TEST(pid) { error_at_line(1, 0, "file", 42, "<message>"); }

    error(0, ENOMEM, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");
    FORKED_TEST(pid) { error(1, ENOMEM, "<message>"); }
    FORKED_TEST(pid) { error_at_line(1, ENOMEM, "file", 42, "<message>"); }

    fflush(stdout);
    printf("\n\n");
    fflush(stdout);

    error_print_progname = print_progname;
    error_one_per_line = 1;

    error(0, 0, "<message>");
    error_at_line(0, 0, "file", 42, "<message>");
    FORKED_TEST(pid) { error(1, 0, "<message>"); }
    FORKED_TEST(pid) { error_at_line(1, 0, "file", 42, "<message>"); }

    error(0, ENOMEM, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");
    FORKED_TEST(pid) { error(1, ENOMEM, "<message>"); }

    error_one_per_line = 0;
    FORKED_TEST(pid) { error_at_line(1, ENOMEM, "file", 42, "<message>"); }

    return EXIT_SUCCESS;
}

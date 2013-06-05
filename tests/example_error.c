/*
 * Test error() and error_at_line(). Non-standard, GNU only.
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

#define _GNU_SOURCE /* for program_invocation_name */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>

void (*error_print_progname)(void);


static void print_prognmae(void) {
    fprintf(stderr, "PROG");
}


int main(int argc, char **argv) {
    program_invocation_name = "./example_error";

    error(0, 0, "<message>");
    error_at_line(0, 0, "file", 42, "<message>");

    error(0, ENOMEM, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");

    error_print_progname = print_prognmae;
    error_one_per_line = 1;

    error(0, 0, "<message>");
    error_at_line(0, 0, "file", 42, "<message>");

    error(0, ENOMEM, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");
    error_at_line(0, ENOMEM, "file", 42, "<message>");

    /* Exit codes are not tested. */

    return EXIT_SUCCESS;
}

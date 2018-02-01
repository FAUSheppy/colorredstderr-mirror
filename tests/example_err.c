/*
 * Test err(), verr(), ... Non-standard, BSD only.
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

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../src/compiler.h"
#include "example.h"


int main(int argc unused, char **argv unused) {
    pid_t pid;

    FORKED_TEST(pid) { errno = ENOMEM; err(0, "error: %s", "message"); }
    FORKED_TEST(pid) { errno = ENOMEM; err(1, "error: %s", "message"); }

    FORKED_TEST(pid) { errx(0, "error: %s", "message"); }
    FORKED_TEST(pid) { errx(1, "error: %s", "message"); }

    errno = ENOMEM;
    warn("warning: %s", "message");
    warnx("warning: %s", "message");

    /* v*() functions are implicitly tested - the implementation uses them. */

    printf("\n");
    return EXIT_SUCCESS;
}

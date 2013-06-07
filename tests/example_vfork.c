/*
 * Test issues with vfork().
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    pid_t pid;

    fprintf(stderr, "Before vfork().\n");

    pid = vfork();
    if (pid == 0) {
        /* This violates the POSIX standard! The "child" is only allowed to
         * modify the result of vfork(), e.g. the pid variable. Some programs
         * (e.g. gdb) do it anyway so we have to workaround it. */
        dup2(STDOUT_FILENO, STDERR_FILENO);

        _exit(2);
    }

    fprintf(stderr, "After vfork().\n");
    puts("");

    return EXIT_SUCCESS;
}

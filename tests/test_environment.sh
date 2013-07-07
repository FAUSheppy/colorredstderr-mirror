#!/bin/sh

# Copyright (C) 2013  Simon Ruderich
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

test "x$srcdir" = x && srcdir=.
. "$srcdir/lib.sh"

# Test unexpected values for COLORED_STDERR_PRIVATE_FDS environment variable.

# Empty fields.
fds=
test_program          example example_environment_empty
test_program_subshell example example_environment_empty
fds=,,,
test_program          example example_environment_empty
test_program_subshell example example_environment_empty
fds=,,,2
test_program          example example_environment_empty
test_program_subshell example example_environment_empty
fds=2,,,
test_program          example example_environment
test_program_subshell example example_environment

# Invalid fds.
fds=-20,-30
test_program          example example_environment_empty
test_program_subshell example example_environment_empty
fds=-20,-30,2,
test_program          example example_environment
test_program_subshell example example_environment
fds=-20,-30,2,-1,
test_program          example example_environment
test_program_subshell example example_environment

# Test COLORED_STDERR_FDS overwrites COLORED_STDERR_PRIVATE_FDS. Additional
# tests in example_exec.

fds=
COLORED_STDERR_FDS=2,
export COLORED_STDERR_FDS
test_program          example example_environment
test_program_subshell example example_environment

fds=2,
COLORED_STDERR_FDS=
export COLORED_STDERR_FDS
test_program          example example_environment_empty
test_program_subshell example example_environment_empty

unset COLORED_STDERR_FDS


# Test COLORED_STDERR_IGNORED_BINARIES.

if test -x /proc/self/exe; then
    COLORED_STDERR_IGNORED_BINARIES="$abs_builddir/example"
    export COLORED_STDERR_IGNORED_BINARIES
    test_program          example example_environment_empty
    test_program_subshell example example_environment_empty

    COLORED_STDERR_IGNORED_BINARIES=",some,other,path,,"
    export COLORED_STDERR_IGNORED_BINARIES
    test_program          example example_environment
    test_program_subshell example example_environment
fi

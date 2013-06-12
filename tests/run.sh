#!/bin/sh

# Test suite for coloredstderr.

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


set -e


# The tests fail if running under coloredstderr because the tests redirect
# stderr to stdout which is detected by coloredstderr :D (and not colored as a
# result). Therefore remove LD_PRELOAD and re-exec the test.
if test -n "$LD_PRELOAD"; then
    unset LD_PRELOAD
    exec "$0"
fi

test "x$srcdir" = x && srcdir=.
. "$srcdir/lib.sh"


# Use valgrind to run the tests if it's available.
valgrind_cmd=
if type valgrind >/dev/null 2>&1; then
    valgrind_cmd='valgrind --quiet --error-exitcode=1'
fi


# Make sure we don't write to non-ttys by default.
force_write=
test_script example_noforce.sh
force_write=1

test_script example_simple.sh
test_script example_redirects.sh
test_program example
test_program example_exec
test -x "$builddir/example_vfork" && test_program example_vfork
test -x "$builddir/example_error" && test_program example_error

test_script_subshell example_simple.sh
test_script_subshell example_redirects.sh
test_program_subshell example
test_program_subshell example_exec
test -x "$builddir/example_vfork" && test_program_subshell example_vfork
test -x "$builddir/example_error" && test_program_subshell example_error

# Necessary in case the test -x evaluates to false.
true

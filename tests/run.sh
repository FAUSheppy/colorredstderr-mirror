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


test "x$srcdir" = x && srcdir=.
. "$srcdir/lib.sh"


# Make sure we don't write to non-ttys by default.
force_write=
test_script example-noforce.sh
force_write=1

test_script example-simple.sh
test_script example-redirects.sh
test_program example
test_program example_vfork

test_script_subshell example-simple.sh
test_script_subshell example-redirects.sh
test_program_subshell example
test_program_subshell example_vfork

#!/bin/sh

# Copyright (C) 2013-2018  Simon Ruderich
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


echo write to stderr 1 >&2
echo write to stdout 1

(
    echo write to stdout which gets redirected to stderr
) >&2

echo write to stdout 2

(
    echo write to stderr which gets redirected to stdout >&2
) 2>&1

echo write to stdout 3

(
    (
        echo another redirect to stderr
    ) >&3
) 3>&2

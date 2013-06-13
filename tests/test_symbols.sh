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

# Check if all hooked functions are actually available in the binary.
symbols=
symbols="$symbols write fwrite"
symbols="$symbols fputs fputc _IO_putc putchar puts"
symbols="$symbols printf fprintf vprintf vfprintf"
symbols="$symbols __printf_chk __fprintf_chk __vprintf_chk __vfprintf_chk"
symbols="$symbols fwrite_unlocked fputs_unlocked fputc_unlocked putc_unlocked putchar_unlocked"
symbols="$symbols perror"
if test -x "$builddir/example_error"; then
    symbols="$symbols error error_at_line"
fi
symbols="$symbols dup dup2 dup3 fcntl close fclose"
if test -x "$builddir/example_vfork"; then
    symbols="$symbols vfork"
fi
symbols="$symbols execve execl execlp execle execv execvp"

output="output-$$"
nm -g -P "$library" > "$output"
for x in $symbols; do
    grep "^$x T " "$output" >/dev/null 2>&1 || die "symbol $x missing"
done
rm "$output"

# Library for the test suite.

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


# Allow running the script directly without running `make check`.
test "x$builddir" = x && builddir=.
test "x$EGREP" = x && EGREP='grep -E'

# In case we are called with LD_PRELOAD already set.
unset LD_PRELOAD
# Clean locale for reproducible tests.
LC_ALL=C
unset LANGUAGE


die() {
    echo "$@" >&2
    exit 1
}

get_library_path() {
    # Get name of the real library file from libtool's .la file.
    dlname=`$EGREP "^dlname='" "$builddir/../src/libcoloredstderr.la"` \
        || die 'dlname not found'
    dlname=`echo "$dlname" | sed "s/^dlname='//; s/'$//"`

    library="$builddir/../src/.libs/$dlname"
    test -e "$library" || die "'$library' not found"

    echo "`pwd`/$library"
}

library=`get_library_path`
force_write=1


run_test() {
    printf '%s' "Running test '$*' .. "

    testcase="$1"
    expected="$2"
    shift
    shift

    (
        # Standard setup.
        LD_PRELOAD="$library"
        COLORED_STDERR_FDS=2,
        export LD_PRELOAD
        export COLORED_STDERR_FDS

        # Change pre/post strings for simpler testing.
        COLORED_STDERR_PRE='>STDERR>'
        COLORED_STDERR_POST='<STDERR<'
        export COLORED_STDERR_PRE
        export COLORED_STDERR_POST
        # And force writes to a file (unless we are testing the force).
        if test "x$force_write" != x; then
            COLORED_STDERR_FORCE_WRITE=1
            export COLORED_STDERR_FORCE_WRITE
        fi

        $valgrind_cmd "$@" "$testcase" > output 2>&1
    )

    diff -u "$expected" output \
        || die 'failed!'
    rm output
    echo 'passed.'
}

test_script() {
    testcase="$1"
    shift
    run_test "$srcdir/$testcase" "$srcdir/$testcase.expected" "$@"
}
test_script_subshell() {
    test_script "$1" bash -c 'bash $1' ''
}
test_program() {
    testcase="$1"
    shift
    run_test "$builddir/$testcase" "$srcdir/$testcase.expected" "$@"
}
test_program_subshell() {
    test_program "$1" sh -c '$1' ''
}

# Library for the test suite.

# Copyright (C) 2013-2015  Simon Ruderich
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


# Allow running the script directly without running `make check`.
test "x$builddir" = x && builddir=.
test "x$abs_builddir" = x && abs_builddir="`pwd`"
test "x$EGREP" = x && EGREP='grep -E'

# The tests fail if running under coloredstderr because the tests redirect
# stderr to stdout which is detected by coloredstderr :D (and not colored as a
# result). Therefore remove LD_PRELOAD and re-exec the test.
if test -n "$LD_PRELOAD"; then
    unset LD_PRELOAD
    exec "$0"
fi

# Use valgrind to run the tests if it's available.
valgrind_cmd=
if type valgrind >/dev/null 2>&1; then
    valgrind_cmd='valgrind --quiet --error-exitcode=1'
fi

# Clean locale for reproducible tests.
LC_ALL=C
unset LANGUAGE

# Clear user defined variables.
unset COLORED_STDERR_FDS
unset COLORED_STDERR_FORCE_WRITE
# Set default COLORED_STDERR_PRIVATE_FDS value.
fds=2,


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

    output="output-$$"

    (
        # Standard setup.
        LD_PRELOAD="$library"
        COLORED_STDERR_PRIVATE_FDS="$fds"
        export LD_PRELOAD
        export COLORED_STDERR_PRIVATE_FDS

        # Change pre/post strings for simpler testing.
        COLORED_STDERR_PRE='>STDERR>'
        COLORED_STDERR_POST='<STDERR<'
        export COLORED_STDERR_PRE
        export COLORED_STDERR_POST
        # And force writes to a file (unless we are testing the force).
        if test -n "$force_write"; then
            COLORED_STDERR_FORCE_WRITE=1
            export COLORED_STDERR_FORCE_WRITE
        fi

        $valgrind_cmd "$@" "$testcase" > "$output" 2>&1
    )

    # Some sed implementations (e.g. on FreeBSD 9.1) always append a trailing
    # newline. Add "EOF" to detect if the real output had one.
    echo EOF >> "$output"

    # Merge continuous regions of colored output. The exact calls don't matter
    # as long as the output is colored.
    sed 's/<STDERR<>STDERR>//g' < "$output" > "$output.tmp"
    mv "$output.tmp" "$output"

    diff -u "$expected" "$output" \
        || die 'failed!'
    rm "$output"
    echo 'passed.'
}

test_script() {
    testcase="$1"
    expected="$2"
    # shift || true is not enough for dash.
    test $# -ge 2 && shift
    shift

    if test -z "$expected"; then
        expected="$testcase"
    fi
    run_test "$srcdir/$testcase" "$srcdir/$expected.expected" "$@"
}
test_script_subshell() {
    test_script "$1" "$2" sh -c 'sh $1' ''
}
test_program() {
    testcase="$1"
    expected="$2"
    test $# -ge 2 && shift
    shift

    if test -z "$expected"; then
        expected="$testcase"
    fi
    run_test "$builddir/$testcase" "$srcdir/$expected.expected" "$@"
}
test_program_subshell() {
    test_program "$1" "$2" sh -c '$1' ''
}

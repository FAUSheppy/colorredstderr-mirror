TESTS = run.sh
check_PROGRAMS = example example_exec example_vfork
example_SOURCES = example.c
example_exec_SOURCES = example_exec.c
example_vfork_SOURCES = example_vfork.c

if HAVE_ERROR_H
    check_PROGRAMS += example_error
    example_error_SOURCES = example_error.c
endif

dist_check_SCRIPTS = run.sh lib.sh
dist_check_DATA = example.expected \
                  example_error.expected \
                  example_exec.expected \
                  example_vfork.expected \
                  example-noforce.sh \
                  example-noforce.sh.expected \
                  example-redirects.sh \
                  example-redirects.sh.expected \
                  example-simple.sh \
                  example-simple.sh.expected

# Used by run.sh.
export EGREP

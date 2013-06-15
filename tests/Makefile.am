# Default since automake 1.13, necessary for older versions.
AUTOMAKE_OPTIONS = color-tests parallel-tests

TESTS = test_environment.sh \
        test_example.sh \
        test_exec.sh \
        test_noforce.sh \
        test_redirects.sh \
        test_simple.sh \
        test_symbols.sh
check_PROGRAMS = example example_exec
example_SOURCES = example.c
example_exec_SOURCES = example_exec.c

if HAVE_ERR_H
    TESTS += test_err.sh
    check_PROGRAMS += example_err
    example_error_SOURCES = example_err.c
endif
if HAVE_ERROR_H
    TESTS += test_error.sh
    check_PROGRAMS += example_error
    example_error_SOURCES = example_error.c
endif
if HAVE_VFORK
    TESTS += test_vfork.sh
    check_PROGRAMS += example_vfork
    example_vfork_SOURCES = example_vfork.c
endif

dist_check_SCRIPTS = $(TESTS) lib.sh
dist_check_DATA = example.h \
                  example.expected \
                  example_environment.expected \
                  example_environment_empty.expected \
                  example_err.expected \
                  example_error.expected \
                  example_exec.expected \
                  example_noforce.sh \
                  example_noforce.sh.expected \
                  example_redirects.sh \
                  example_redirects.sh.expected \
                  example_simple.sh \
                  example_simple.sh.expected \
                  example_vfork.expected

# Used by lib.sh.
export EGREP

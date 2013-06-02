TESTS = run.sh
check_PROGRAMS = example example_vfork
example_SOURCES = example.c
example_vfork_SOURCES = example_vfork.c

dist_check_SCRIPTS = run.sh lib.sh
dist_check_DATA = example.expected \
                  example_vfork.expected \
                  example-noforce.sh \
                  example-noforce.sh.expected \
                  example-redirects.sh \
                  example-redirects.sh.expected \
                  example-simple.sh \
                  example-simple.sh.expected

# Used by run.sh.
export EGREP

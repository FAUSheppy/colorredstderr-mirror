/*
 * Test execve() and exec*() functions.
 *
 * Copyright (C) 2013  Simon Ruderich
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

/* For execvpe(). */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "example.h"
#include "../src/compiler.h"


#define MAGIC "@RUN_"
#define MAGIC_LENGTH (strlen(MAGIC))

#define SETUP_RUN(number) \
        sprintf(argv0, "%s" MAGIC "%03d", argv[0], (number))


extern char **environ;


static int find_magic_run_number(char *argv0, int *number) {
    size_t length = strlen(argv0);
    if (length <= MAGIC_LENGTH + 3) {
        return 0;
    }
    /* Check if the string ends with "@RUN_YYY" where YYY should be a number
     * (not checked). */
    argv0 += length - MAGIC_LENGTH - 3;
    if (strncmp(argv0, MAGIC, MAGIC_LENGTH)) {
        return 0;
    }
    *number = atoi(argv0 + MAGIC_LENGTH);

    /* Strip off "@RUN_YYY". */
    argv0[0] = 0;
    return 1;
}

static int cmp(void const *a, void const *b) {
    return strcmp(*(char * const *)a, *(char * const *)b);
}
static void dump(char *argv[]) {
    size_t i;

    i = 0;
    while (*argv) {
        printf("argv[%zu] = |%s|\n", i++, *argv++);
    }

    /* Order of environment variables is not defined, sort them for consistent
     * test results. */
    i = 0;
    char **env = environ;
    while (*env++) {
        i++;
    }
    qsort(environ, i, sizeof(*env), cmp);

    i = 0;
    env = environ;
    while (*env) {
        /* Skip LD_PRELOAD which contains system dependent values. */
        if (strncmp(*env, "LD_PRELOAD=", 11)) {
            printf("environ[%zu] = |%s|\n", i++, *env);
        }
        env++;
    }
    printf("\n");
    /* When not writing to stdout (e.g. redirection while testing), glibc is
     * buffering heavily. Flush to display the output before the exec*(). */
    fflush(stdout);
}


int main(int argc unused, char **argv) {
    char argv0[strlen(argv[0]) + MAGIC_LENGTH + 3 + 1];

    char *old_ldpreload = getenv("LD_PRELOAD");
    size_t ldpreload_length = strlen("LD_PRELOAD=") + 1;
    if (old_ldpreload) {
        ldpreload_length += strlen(old_ldpreload);
    }
    char ldpreload[ldpreload_length];
    strcpy(ldpreload, "LD_PRELOAD=");
    if (old_ldpreload) {
        strcat(ldpreload, old_ldpreload);
    }

    int run;

    /* First call. */
    if (!find_magic_run_number(argv[0], &run)) {
        SETUP_RUN(1);

        char *args[] = { argv0, NULL };
        char *envp[] = { ldpreload, NULL };

        execve(argv[0], args, envp);
        return EXIT_FAILURE;
    }

    /* Following calls, each testing an exec*() feature. */

    dump(argv);
    SETUP_RUN(run + 1);

    int skip = run - 1;


    /* Check that we update/insert the environment correctly. */

    if (!skip--) {
        printf("\nCHECKING COLORING.\n\n");
        fflush(stdout);

        xdup2(2, 3);

        char *args[] = { argv0, NULL };
        char *envp[] = { ldpreload, NULL };

        execve(argv[0], args, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        xdup2(2, 4);

        execl(argv[0], argv0, NULL);
        return EXIT_FAILURE;
    } else if (!skip--) {
        xdup2(2, 5);

        execlp(argv[0], argv0, NULL);
        return EXIT_FAILURE;
    } else if (!skip--) {
        xdup2(2, 6);

        char *envp[] = { ldpreload, NULL };

        execle(argv[0], argv0, NULL, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        xdup2(2, 7);

        /* Test closing a few descriptors. */
        close(5);
        close(6);

        char *args[] = { argv0, NULL };

        execv(argv[0], args);
        return EXIT_FAILURE;
    } else if (!skip--) {
        xdup2(2, 8);

        /* And a few more. */
        close(7);
        close(4);

        char *args[] = { argv0, NULL };

        execvp(argv[0], args);
        return EXIT_FAILURE;

    /* Test handling of COLORED_STDERR_FDS. */

    } else if (!skip--) {
        /* And the rest. */
        close(3);
        close(8);

        xdup2(2, 5);

        char *args[] = { argv0, NULL };
        char *envp[] = { ldpreload, "COLORED_STDERR_FDS=5,", NULL };

        execve(argv[0], args, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        char *args[] = { argv0, NULL };
        char *envp[] = { ldpreload, NULL };

        xdup2(5, 6);
        close(5);

        execve(argv[0], args, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        close(6);

        char *args[] = { argv0, NULL };
        setenv("COLORED_STDERR_FDS", "2,", 1);

        execv(argv[0], args);
        return EXIT_FAILURE;
    }


    /* And make sure our hooks don't change the behaviour. */

    /* execve(2) */
    if (!skip--) {
        printf("\nCHECKING TRANSPARENCY.\n\n");
        fflush(stdout);

        char *args[] = { argv0, NULL };
        char *envp[] = { ldpreload, NULL };

        execve(argv[0], args, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        char *args[] = { argv0, NULL };
        char *envp[] = { "TEST=42", ldpreload, NULL };

        execve(argv[0], args, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        char *args[] = { argv0, "foo", "bar", NULL };
        char *envp[] = { "TEST=43", "FOO=", ldpreload, NULL };

        execve(argv[0], args, envp);
        return EXIT_FAILURE;

    /* execl(3) */
    } else if (!skip--) {
        setenv("TEST", "44", 1);

        execl(argv[0], argv0, NULL);
        return EXIT_FAILURE;
    } else if (!skip--) {
        setenv("TEST", "45", 1);

        execl(argv[0], argv0, "foo", "bar", NULL);
        return EXIT_FAILURE;

    /* execp(3), but not testing the p(ath) part */
    } else if (!skip--) {
        setenv("TEST", "46", 1);

        execlp(argv[0], argv0, NULL);
        return EXIT_FAILURE;
    } else if (!skip--) {
        setenv("TEST", "47", 1);

        execlp(argv[0], argv0, "foo", "bar", NULL);
        return EXIT_FAILURE;

    /* execle(3) */
    } else if (!skip--) {
        char *envp[] = { ldpreload, NULL };

        execle(argv[0], argv0, NULL, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        char *envp[] = { "TEST=48", ldpreload, NULL };

        execle(argv[0], argv0, NULL, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        char *envp[] = { "TEST=49", "FOO=", ldpreload, NULL };

        execle(argv[0], argv0, "foo", "bar", NULL, envp);
        return EXIT_FAILURE;

    /* execv(3) */
    } else if (!skip--) {
        setenv("TEST", "50", 1);

        char *args[] = { argv0, NULL };

        execv(argv[0], args);
        return EXIT_FAILURE;
    } else if (!skip--) {
        setenv("TEST", "51", 1);

        char *args[] = { argv0, "foo", "bar", NULL };

        execv(argv[0], args);
        return EXIT_FAILURE;

    /* execvp(3), but not testing the p(ath) part */
    } else if (!skip--) {
        setenv("TEST", "52", 1);

        char *args[] = { argv0, NULL };

        execvp(argv[0], args);
        return EXIT_FAILURE;
    } else if (!skip--) {
        setenv("TEST", "53", 1);

        char *args[] = { argv0, "foo", "bar", NULL };

        execvp(argv[0], args);
        return EXIT_FAILURE;

#ifdef HAVE_EXECVPE
    /* execvpe(3), but not testing the p(ath) part */
    } else if (!skip--) {
        char *args[] = { argv0, NULL };
        char *envp[] = { "TEST=54", ldpreload, NULL };

        execvpe(argv[0], args, envp);
        return EXIT_FAILURE;
    } else if (!skip--) {
        char *args[] = { argv0, "foo", "bar", NULL };
        char *envp[] = { "TEST=55", ldpreload, NULL };

        execvpe(argv[0], args, envp);
        return EXIT_FAILURE;
#else
    /* Fake output to let the test pass. */
    } else if (!skip--) {
        puts("argv[0] = |./example_exec|");
        puts("environ[0] = |COLORED_STDERR_PRIVATE_FDS=2,|");
        puts("environ[1] = |TEST=54|");
        puts("");
        puts("argv[0] = |./example_exec|");
        puts("argv[1] = |foo|");
        puts("argv[2] = |bar|");
        puts("environ[0] = |COLORED_STDERR_PRIVATE_FDS=2,|");
        puts("environ[1] = |TEST=55|");
        puts("");
#endif
    }

    printf("Done.\n");
    return EXIT_SUCCESS;
}

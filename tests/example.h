/*
 * Helper functions/macros for example files.
 *
 * Copyright (C) 2013-2014  Simon Ruderich
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


#define FORKED_TEST(pid) \
    pid = fork(); \
    if (pid == -1) { \
        perror("fork"); \
        exit(EXIT_FAILURE); \
    } else if (pid != 0) { \
        int status; \
        if (waitpid(pid, &status, 0) == -1) { \
            perror("waitpid"); \
            exit(EXIT_FAILURE); \
        } \
        if (WIFEXITED(status)) { \
            printf("exit code: %d\n", WEXITSTATUS(status)); \
        } else { \
            printf("child terminated!\n"); \
        } \
        fflush(stdout); \
    } else

static ssize_t xwrite(int fd, void const *buf, size_t count) {
    ssize_t result = write(fd, buf, count);
    if (result == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    /* Ignore short writes here. Doesn't matter for test cases. */
    return result;
}

static int xdup2(int oldfd, int newfd) {
    int result = dup2(oldfd, newfd);
    if (result == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    return result;
}

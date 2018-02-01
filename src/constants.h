/*
 * Global constants and defines.
 *
 * Copyright (C) 2013-2018  Simon Ruderich
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

#ifndef CONSTANTS_H
#define CONSTANTS_H 1

/* Names of used environment variables. */
#define ENV_NAME_FDS              "COLORED_STDERR_FDS"
#define ENV_NAME_PRE_STRING       "COLORED_STDERR_PRE"
#define ENV_NAME_POST_STRING      "COLORED_STDERR_POST"
#define ENV_NAME_FORCE_WRITE      "COLORED_STDERR_FORCE_WRITE"
#define ENV_NAME_IGNORED_BINARIES "COLORED_STDERR_IGNORED_BINARIES"
#define ENV_NAME_PRIVATE_FDS      "COLORED_STDERR_PRIVATE_FDS"

/* Strings written before/after each matched function. */
#define DEFAULT_PRE_STRING  "\033[31m" /* red */
#define DEFAULT_POST_STRING "\033[0m"  /* default */

/* Number of elements to allocate statically. Highest descriptor observed in
 * normal use was 255 (by bash), which yielded this limit to prevent
 * unnecessary calls to malloc() whenever possible. */
#define TRACKFDS_STATIC_COUNT 256
/* Number of new elements to allocate per realloc(). */
#define TRACKFDS_REALLOC_STEP 10

#ifdef DEBUG
# define DEBUG_FILE "colored_stderr_debug_log.txt"
#endif
#ifdef WARNING
/* Created in the user's home directory, appends to existing file. */
# define WARNING_FILE "colored_stderr_warning_log.txt"
#endif

#endif

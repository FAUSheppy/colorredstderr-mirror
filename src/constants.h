/*
 * Global constants and defines.
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

#ifndef CONSTANTS_H
#define CONSTANTS_H 1

/* Names of used environment variables. */
#define ENV_NAME_FDS "COLORED_STDERR_FDS"
#define ENV_NAME_PRE_STRING "COLORED_STDERR_PRE"
#define ENV_NAME_POST_STRING "COLORED_STDERR_POST"
#define ENV_NAME_FORCE_WRITE "COLORED_STDERR_FORCE_WRITE"

/* Strings written before/after each matched function. */
#define DEFAULT_PRE_STRING "\e[91m"
#define DEFAULT_POST_STRING "\e[0m"

/* Number of new elements to allocate per realloc(). */
#define TRACKFDS_REALLOC_STEP 10

#ifdef DEBUG
# define DEBUG_FILE "colored_stderr_debug_log.txt"
#endif

#endif

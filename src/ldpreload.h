/*
 * Helper header for LD_PRELOAD related headers macros. Must be loaded _first_
 * (for RTLD_NEXT)!
 *
 * Copyright (C) 2012-2013  Simon Ruderich
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

#ifndef LDPRELOAD_H
#define LDPRELOAD_H 1

/* Necessary for RTLD_NEXT. */
#define _GNU_SOURCE

/* abort() */
#include <stdlib.h>
/* dlsym() */
#include <dlfcn.h>


/* Load the function name using dlsym() if necessary and store it in pointer.
 * Terminate program on failure. */
#define DLSYM_FUNCTION(pointer, name) \
    if (NULL == (pointer)) { \
        int saved_errnox = errno; \
        char *error; \
        dlerror(); /* Clear possibly existing error. */ \
        \
        *(void **) (&(pointer)) = dlsym(RTLD_NEXT, (name)); \
        \
        if (NULL != (error = dlerror())) { \
            /* Not much we can do. Most likely the other output functions \
             * failed to load too. */ \
            abort(); \
        } \
        errno = saved_errnox; \
    }

#endif

/*
 * Macros to hook functions.
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

#ifndef MACROS_H
#define MACROS_H 1

/* Hook the function by creating a function with the same name. With
 * LD_PRELOAD our function will be preferred. The original function is stored
 * in a static variable (real_*). Any function called in these macros must
 * make sure to restore the errno if it changes it.
 *
 * "Pseudo code" for the following macros. <name> is the name of the hooked
 * function, <fd> is either a file descriptor or a FILE pointer.
 *
 *     if (!real_<name>) {
 *         real_<name> = dlsym_function(<name>);
 *         if (!initialized) {
 *             init_from_environment();
 *         }
 *     }
 *     if (tracked_fds_find(<fd>)) {
 *         if (force_write_to_non_tty) {
 *             handle = 1;
 *         } else {
 *             handle = isatty(<fd>);
 *         }
 *     } else {
 *         handle = 0;
 *     }
 *
 *     if (handle) {
 *         handle_<fd>_pre(<fd>);
 *     }
 *     <type> result = real_<name>(<args>);
 *     if (handle) {
 *         handle_<fd>_post(<fd>);
 *     }
 *     return result;
 */

#define _HOOK_PRE(type, name) \
        int handle; \
        if (!(real_ ## name )) { \
            *(void **) (&(real_ ## name)) = dlsym_function(#name); \
            /* Initialize our data while we're at it. */ \
            if (!initialized) { \
                init_from_environment(); \
            } \
        }
#define _HOOK_PRE_FD(type, name, fd) \
        type result; \
        _HOOK_PRE_FD_(type, name, fd)
#define _HOOK_PRE_FD_(type, name, fd) \
        _HOOK_PRE(type, name) \
        if (tracked_fds_find(fd)) { \
            if (force_write_to_non_tty) { \
                handle = 1; \
            } else { \
                handle = isatty(fd); \
            } \
        } else { \
            handle = 0; \
        } \
        if (handle) { \
            handle_fd_pre(fd); \
        }
#define _HOOK_PRE_FILE(type, name, file) \
        type result; \
        _HOOK_PRE(type, name) \
        if (tracked_fds_find(fileno(file))) { \
            if (force_write_to_non_tty) { \
                handle = 1; \
            } else { \
                handle = isatty(fileno(file)); \
            } \
        } else { \
            handle = 0; \
        } \
        if (handle) { \
            handle_file_pre(file); \
        }
#define _HOOK_POST_FD_(fd) \
        if (handle) { \
            handle_fd_post(fd); \
        }
#define _HOOK_POST_FD(fd) \
        _HOOK_POST_FD_(fd) \
        return result;
#define _HOOK_POST_FILE(file) \
        if (handle) { \
            handle_file_post(file); \
        } \
        return result;


#define HOOK_VOID1(type, name, fd, type1, arg1) \
    static type (*real_ ## name)(type1); \
    type name(type1 arg1) { \
        _HOOK_PRE_FD_(type, name, fd) \
        real_ ## name(arg1); \
        _HOOK_POST_FD_(fd) \
    }

#define HOOK_FD3(type, name, fd, type1, arg1, type2, arg2, type3, arg3) \
    static type (*real_ ## name)(type1, type2, type3); \
    type name(type1 arg1, type2 arg2, type3 arg3) { \
        _HOOK_PRE_FD(type, name, fd) \
        result = real_ ## name(arg1, arg2, arg3); \
        _HOOK_POST_FD(fd) \
    }

#define HOOK_FILE1(type, name, file, type1, arg1) \
    static type (*real_ ## name)(type1); \
    type name(type1 arg1) { \
        _HOOK_PRE_FILE(type, name, file) \
        result = real_ ## name(arg1); \
        _HOOK_POST_FILE(file) \
    }
#define HOOK_FILE2(type, name, file, type1, arg1, type2, arg2) \
    static type (*real_ ## name)(type1, type2); \
    type name(type1 arg1, type2 arg2) { \
        _HOOK_PRE_FILE(type, name, file) \
        result = real_ ## name(arg1, arg2); \
        _HOOK_POST_FILE(file) \
    }
#define HOOK_FILE3(type, name, file, type1, arg1, type2, arg2, type3, arg3) \
    static type (*real_ ## name)(type1, type2, type3); \
    type name(type1 arg1, type2 arg2, type3 arg3) { \
        _HOOK_PRE_FILE(type, name, file) \
        result = real_ ## name(arg1, arg2, arg3); \
        _HOOK_POST_FILE(file) \
    }
#define HOOK_FILE4(type, name, file, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
    static type (*real_ ## name)(type1, type2, type3, type4); \
    type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4) { \
        _HOOK_PRE_FILE(type, name, file) \
        result = real_ ## name(arg1, arg2, arg3, arg4); \
        _HOOK_POST_FILE(file) \
    }

#define HOOK_VAR_FILE1(type, name, file, func, type1, arg1) \
    static type (*real_ ## func)(type1, va_list); \
    type name(type1 arg1, ...) { \
        va_list ap; \
        _HOOK_PRE_FILE(type, func, file) \
        va_start(ap, arg1); \
        result = real_ ## func(arg1, ap); \
        va_end(ap); \
        _HOOK_POST_FILE(file) \
    }
#define HOOK_VAR_FILE2(type, name, file, func, type1, arg1, type2, arg2) \
    static type (*real_ ## func)(type1, type2, va_list); \
    type name(type1 arg1, type2 arg2, ...) { \
        va_list ap; \
        _HOOK_PRE_FILE(type, func, file) \
        va_start(ap, arg2); \
        result = real_ ## func(arg1, arg2, ap); \
        va_end(ap); \
        _HOOK_POST_FILE(file) \
    }
#define HOOK_VAR_FILE3(type, name, file, func, type1, arg1, type2, arg2, type3, arg3) \
    static type (*real_ ## func)(type1, type2, type3, va_list); \
    type name(type1 arg1, type2 arg2, type3 arg3, ...) \
    { \
        va_list ap; \
        _HOOK_PRE_FILE(type, func, file) \
        va_start(ap, arg3); \
        result = real_ ## func(arg1, arg2, arg3, ap); \
        va_end(ap); \
        _HOOK_POST_FILE(file) \
    }

#endif

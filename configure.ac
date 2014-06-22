dnl Copyright (C) 2013-2014  Simon Ruderich
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.


AC_PREREQ([2.69])
AC_INIT([coloredstderr],[0.2],[simon@ruderich.org])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/coloredstderr.c])

AM_PROG_AR
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_PREREQ([2.4])
LT_INIT([disable-static])

AC_PROG_CC
dnl Used by test suite.
AC_PROG_SED
AC_PROG_EGREP

if test "x$GCC" = xyes; then
    CFLAGS="-Wall -Wextra -Wconversion $CFLAGS"
fi

AC_CHECK_HEADERS([err.h error.h])
AC_CHECK_HEADERS([fcntl.h],
                 [],[AC_MSG_ERROR([header is required])])

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_C_INLINE
AX_C___ATTRIBUTE__
AX_TLS

AC_CHECK_MEMBER([struct _IO_FILE._fileno],
                [AC_DEFINE([HAVE_STRUCT__IO_FILE__FILENO], 1,
                           [Define to 1 if `_fileno' is a member of `struct _IO_FILE'.])],
                [],[[#include <libio.h>]]) dnl ' fix for vim syntax coloring

AC_FUNC_FORK
AC_CHECK_FUNCS([memmove setenv],
               [],[AC_MSG_ERROR([function is required])])
AC_CHECK_FUNCS([execvpe])
dnl These are not in POSIX.
AC_CHECK_FUNCS([fwrite_unlocked fputs_unlocked fputc_unlocked])
dnl Internal functions in libc implementations which must be hooked.
AC_CHECK_FUNCS([__overflow __swbuf])

dnl Thanks to gperftools' configure.ac (https://code.google.com/p/gperftools).
AC_MSG_CHECKING([for __builtin_expect])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[return __builtin_expect(main != 0, 1)])],
               [AC_DEFINE([HAVE___BUILTIN_EXPECT], 1,
                          [Define to 1 if the compiler supports __builtin_expect().])
                AC_MSG_RESULT([yes])],
               [AC_MSG_RESULT([no])])

AC_SEARCH_LIBS([dlsym], [dl], [], [AC_MSG_ERROR([dlsym() is required])])

AC_ARG_ENABLE([warnings],
              [AS_HELP_STRING([--enable-warnings],[enable warning output])],
              [if test "x$enableval" = xyes; then
                   AC_DEFINE([WARNING], 1, [Define to 1 enable warning output.])
               fi])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],[enable debug output])],
              [if test "x$enableval" = xyes; then
                   AC_DEFINE([DEBUG], 1, [Define to 1 enable debug output.])
                   dnl DEBUG implies WARNING
                   AC_DEFINE([WARNING], 1)
               fi])

dnl Used in tests/Makefile.am to build the test only if function is available.
AM_CONDITIONAL([HAVE_ERR_H],[test "x$ac_cv_header_err_h" = xyes])
AM_CONDITIONAL([HAVE_ERROR_H],[test "x$ac_cv_header_error_h" = xyes])
AM_CONDITIONAL([HAVE_VFORK],[test "x$ac_cv_func_vfork_works" = xyes])

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_OUTPUT

if test x"$ac_cv_tls" = x"none"; then
    AC_MSG_WARN([thread-local storage not supported by compiler, \
possible race condition in threaded programs])
fi

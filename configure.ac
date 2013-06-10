dnl Copyright (C) 2013  Simon Ruderich
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
AC_INIT([coloredstderr],[0.1],[simon@ruderich.org])

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

AC_CHECK_HEADERS([fcntl.h error.h])

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_C_INLINE

AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 memmove setenv strdup])

dnl Thanks to gperftools' configure.ac (https://code.google.com/p/gperftools).
AC_MSG_CHECKING([for __builtin_expect])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],[return __builtin_expect(main != 0, 1)])],
               [AC_DEFINE([HAVE___BUILTIN_EXPECT], 1,
                          [Define to 1 if the compiler supports __builtin_expect().])
                AC_MSG_RESULT([yes])],
               [AC_MSG_RESULT([no])])

AC_SEARCH_LIBS([dlsym], [dl], [], [AC_MSG_ERROR([dlsym() is required])])

AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],[enable debug output])],
              [if test "x$enableval" = xyes; then
                   AC_DEFINE([DEBUG], 1, [Define to enable debug output.])
               fi])

dnl Used in tests/Makefile.am to build the test only if error() is available.
AM_CONDITIONAL([HAVE_ERROR_H],[test "x$ac_cv_header_error_h" = xyes])

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_OUTPUT

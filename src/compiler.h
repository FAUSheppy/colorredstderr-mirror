/*
 * Compiler specific macros.
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

#ifndef COMPILER_H
#define COMPILER_H 1

#ifdef HAVE___ATTRIBUTE__
/* Prevent/force inlining. Used to improve performance. */
# define noinline      __attribute__((noinline))
# define always_inline __attribute__((always_inline))
/* Unused parameter. */
# define unused        __attribute__((unused))
/* Mark the function protected, which means it can't be overwritten by other
 * modules (libraries), e.g. with LD_PRELOAD); otherwise same as the default
 * visibility. This causes the compiler not use the PLT (and no relocations)
 * for local calls from inside this module; but the symbols are still
 * exported. This is faster and in this case prevents useless lookups as we
 * hook those functions and nobody else should modify them. Not strictly
 * necessary, but nice to have. */
# define visibility_protected __attribute__((visibility("protected")))
#else
# define noinline
# define always_inline
# define unused
# define visibility_protected
#endif

/* Branch prediction information for the compiler. */
#ifdef HAVE___BUILTIN_EXPECT
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   x
# define unlikely(x) x
#endif

#endif

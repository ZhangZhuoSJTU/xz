// SPDX-License-Identifier: 0BSD

///////////////////////////////////////////////////////////////////////////////
//
/// \file       tuklib_common.h
/// \brief      Common definitions for tuklib modules
//
//  Author:     Lasse Collin
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TUKLIB_COMMON_H
#define TUKLIB_COMMON_H

// The config file may be replaced by a package-specific file.
// It should include at least stddef.h, stdbool.h, inttypes.h, and limits.h.
#include "tuklib_config.h"

// TUKLIB_SYMBOL_PREFIX is prefixed to all symbols exported by
// the tuklib modules. If you use a tuklib module in a library,
// you should use TUKLIB_SYMBOL_PREFIX to make sure that there
// are no symbol conflicts in case someone links your library
// into application that also uses the same tuklib module.
#ifndef TUKLIB_SYMBOL_PREFIX
#	define TUKLIB_SYMBOL_PREFIX
#endif

#define TUKLIB_CAT_X(a, b) a ## b
#define TUKLIB_CAT(a, b) TUKLIB_CAT_X(a, b)

#ifndef TUKLIB_SYMBOL
#	define TUKLIB_SYMBOL(sym) TUKLIB_CAT(TUKLIB_SYMBOL_PREFIX, sym)
#endif

#ifndef TUKLIB_DECLS_BEGIN
#	ifdef __cplusplus
#		define TUKLIB_DECLS_BEGIN extern "C" {
#	else
#		define TUKLIB_DECLS_BEGIN
#	endif
#endif

#ifndef TUKLIB_DECLS_END
#	ifdef __cplusplus
#		define TUKLIB_DECLS_END }
#	else
#		define TUKLIB_DECLS_END
#	endif
#endif

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#	define TUKLIB_GNUC_REQ(major, minor) \
		((__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)) \
			|| __GNUC__ > (major))
#else
#	define TUKLIB_GNUC_REQ(major, minor) 0
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define tuklib_attr_format_printf(fmt_index, args_index) \
		__attribute__((__format__(__printf__, fmt_index, args_index)))
#else
#	define tuklib_attr_format_printf(fmt_index, args_index)
#endif

// tuklib_attr_noreturn is used to mark functions as non-returning. The
// distinct name avoids interfering with C23 code that uses [[noreturn]].
//
// It must be used at the beginning of a function declaration to work in all
// cases. _Noreturn is used in C11 and newer, including C23, to keep the
// declaration compatible with pre-C23 C standards:
//
//     tuklib_attr_noreturn
//     __attribute__((nonnull(1)))
//     extern void foo(const char *s);
//
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112
#	define tuklib_attr_noreturn _Noreturn
#elif TUKLIB_GNUC_REQ(2, 5)
#	define tuklib_attr_noreturn __attribute__((__noreturn__))
#elif defined(_MSC_VER)
#	define tuklib_attr_noreturn __declspec(noreturn)
#else
#	define tuklib_attr_noreturn
#endif

#if (defined(_WIN32) && !defined(__CYGWIN__)) \
		|| defined(__OS2__) || defined(__MSDOS__)
#	define TUKLIB_DOSLIKE 1
#endif

#endif

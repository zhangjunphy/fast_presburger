//===-- llvm/Support/Compiler.h - Compiler abstraction support --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines several macros, based on the current compiler.  This allows
// use of compiler-specific features in a way that remains portable. This header
// can be included from either C or C++.
//
//===----------------------------------------------------------------------===//

#if __has_builtin(__builtin_expect) || defined(__GNUC__)
#define FP_LIKELY(EXPR) __builtin_expect((bool)(EXPR), true)
#define FP_UNLIKELY(EXPR) __builtin_expect((bool)(EXPR), false)
#else
#define FP_LIKELY(EXPR) (EXPR)
#define FP_UNLIKELY(EXPR) (EXPR)
#endif

/// FP_ATTRIBUTE_ALWAYS_INLINE - On compilers where we have a directive to do
/// so, mark a method "always inline" because it is performance sensitive.
#if __has_attribute(always_inline)
#define FP_ATTRIBUTE_ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FP_ATTRIBUTE_ALWAYS_INLINE __forceinline
#else
#define FP_ATTRIBUTE_ALWAYS_INLINE inline
#endif

/// \macro FP_GNUC_PREREQ
/// Extend the default __GNUC_PREREQ even if glibc's features.h isn't
/// available.
#ifndef FP_GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#  define FP_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >= \
     ((maj) << 20) + ((min) << 10) + (patch))
# elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define FP_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
# else
#  define FP_GNUC_PREREQ(maj, min, patch) 0
# endif
#endif

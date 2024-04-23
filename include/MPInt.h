//===- MPInt.h - MLIR MPInt Class -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is a simple class to represent arbitrary precision signed integers.
// Unlike APInt, one does not have to specify a fixed maximum size, and the
// integer can take on any arbitrary values. This is optimized for small-values
// by providing fast-paths for the cases when the value stored fits in 64-bits.
//
//===----------------------------------------------------------------------===//

#ifndef FP_MPINT_H
#define FP_MPINT_H

#include "ArrayRef.h"
#include "Compiler.h"
#include "SlowMPInt.h"
#include "Math.h"
#include "Utils.h"
#include <cassert>
#include <limits>
#include <numeric>

namespace presburger {

/// Redefine these functions, which operate on 64-bit ints, to also be part of
/// the mlir::presburger namespace. This is useful because this file defines
/// identically-named functions that operate on MPInts, which would otherwie
/// become the only candidates of overload resolution when calling e.g. ceilDiv
/// from the mlir::presburger namespace. So to access the 64-bit overloads, an
/// explict call to mlir::ceilDiv would be required. These using declarations
/// allow overload resolution to transparently call the right function.
// using ::mlir::ceilDiv;
// using ::mlir::floorDiv;
// using ::mlir::mod;

namespace detail {
/// If builtin intrinsics for overflow-checked arithmetic are available,
/// use them. Otherwise, call through to LLVM's overflow-checked arithmetic
/// functionality. Those functions also have such macro-gated uses of intrinsics
/// but they are not always_inlined, which is important for us to achieve
/// high-performance; calling the functions directly would result in a slowdown
/// of 1.15x.
FP_ATTRIBUTE_ALWAYS_INLINE bool addOverflow(int64_t x, int64_t y,
                                            int64_t &result) {
#if __has_builtin(__builtin_add_overflow)
  return __builtin_add_overflow(x, y, &result);
#else
  return AddOverflow(x, y, result);
#endif
}
FP_ATTRIBUTE_ALWAYS_INLINE bool subOverflow(int64_t x, int64_t y,
                                            int64_t &result) {
#if __has_builtin(__builtin_sub_overflow)
  return __builtin_sub_overflow(x, y, &result);
#else
  return SubOverflow(x, y, result);
#endif
}
FP_ATTRIBUTE_ALWAYS_INLINE bool mulOverflow(int64_t x, int64_t y,
                                            int64_t &result) {
#if __has_builtin(__builtin_mul_overflow)
  return __builtin_mul_overflow(x, y, &result);
#else
  return MulOverflow(x, y, result);
#endif
}
} // namespace detail

/// This class provides support for multi-precision arithmetic.
///
/// Unlike APInt, this extends the precision as necessary to prevent overflows
/// and supports operations between objects with differing internal precisions.
///
/// This is optimized for small-values by providing fast-paths for the cases
/// when the value stored fits in 64-bits. We annotate all fastpaths by using
/// the LLVM_LIKELY/LLVM_UNLIKELY annotations. Removing these would result in
/// a 1.2x performance slowdown.
///
/// We always_inline all operations; removing these results in a 1.5x
/// performance slowdown.
///
/// When holdsLarge is true, a SlowMPInt is held in the union. If it is false,
/// the int64_t is held. Using std::variant instead would lead to significantly
/// worse performance.
class MPInt {
private:
  union {
    int64_t valSmall;
    detail::SlowMPInt valLarge;
  };
  unsigned holdsLarge;

  FP_ATTRIBUTE_ALWAYS_INLINE void initSmall(int64_t o) {
    if (FP_UNLIKELY(isLarge()))
      valLarge.detail::SlowMPInt::~SlowMPInt();
    valSmall = o;
    holdsLarge = false;
  }
  FP_ATTRIBUTE_ALWAYS_INLINE void initLarge(const detail::SlowMPInt &o) {
    if (FP_LIKELY(isSmall())) {
      // The data in memory could be in an arbitrary state, not necessarily
      // corresponding to any valid state of valLarge; we cannot call any member
      // functions, e.g. the assignment operator on it, as they may access the
      // invalid internal state. We instead construct a new object using
      // placement new.
      new (&valLarge) detail::SlowMPInt(o);
    } else {
      // In this case, we need to use the assignment operator, because if we use
      // placement-new as above we would lose track of allocated memory
      // and leak it.
      valLarge = o;
    }
    holdsLarge = true;
  }

  FP_ATTRIBUTE_ALWAYS_INLINE explicit MPInt(const detail::SlowMPInt &val)
      : valLarge(val), holdsLarge(true) {}
  FP_ATTRIBUTE_ALWAYS_INLINE bool isSmall() const { return !holdsLarge; }
  FP_ATTRIBUTE_ALWAYS_INLINE bool isLarge() const { return holdsLarge; }
  /// Get the stored value. For getSmall/Large,
  /// the stored value should be small/large.
  FP_ATTRIBUTE_ALWAYS_INLINE int64_t getSmall() const {
    assert(isSmall() &&
           "getSmall should only be called when the value stored is small!");
    return valSmall;
  }
  FP_ATTRIBUTE_ALWAYS_INLINE int64_t &getSmall() {
    assert(isSmall() &&
           "getSmall should only be called when the value stored is small!");
    return valSmall;
  }
  FP_ATTRIBUTE_ALWAYS_INLINE const detail::SlowMPInt &getLarge() const {
    assert(isLarge() &&
           "getLarge should only be called when the value stored is large!");
    return valLarge;
  }
  FP_ATTRIBUTE_ALWAYS_INLINE detail::SlowMPInt &getLarge() {
    assert(isLarge() &&
           "getLarge should only be called when the value stored is large!");
    return valLarge;
  }
  explicit operator detail::SlowMPInt() const {
    if (isSmall())
      return detail::SlowMPInt(getSmall());
    return getLarge();
  }

public:
  FP_ATTRIBUTE_ALWAYS_INLINE explicit MPInt(int64_t val)
      : valSmall(val), holdsLarge(false) {}
  FP_ATTRIBUTE_ALWAYS_INLINE MPInt() : MPInt(0) {}
  FP_ATTRIBUTE_ALWAYS_INLINE ~MPInt() {
    if (FP_UNLIKELY(isLarge()))
      valLarge.detail::SlowMPInt::~SlowMPInt();
  }
  FP_ATTRIBUTE_ALWAYS_INLINE MPInt(const MPInt &o)
      : valSmall(o.valSmall), holdsLarge(false) {
    if (FP_UNLIKELY(o.isLarge()))
      initLarge(o.valLarge);
  }
  FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator=(const MPInt &o) {
    if (FP_LIKELY(o.isSmall())) {
      initSmall(o.valSmall);
      return *this;
    }
    initLarge(o.valLarge);
    return *this;
  }
  FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator=(int x) {
    initSmall(x);
    return *this;
  }
  FP_ATTRIBUTE_ALWAYS_INLINE explicit operator int64_t() const {
    if (isSmall())
      return getSmall();
    return static_cast<int64_t>(getLarge());
  }

  bool operator==(const MPInt &o) const;
  bool operator!=(const MPInt &o) const;
  bool operator>(const MPInt &o) const;
  bool operator<(const MPInt &o) const;
  bool operator<=(const MPInt &o) const;
  bool operator>=(const MPInt &o) const;
  MPInt operator+(const MPInt &o) const;
  MPInt operator-(const MPInt &o) const;
  MPInt operator*(const MPInt &o) const;
  MPInt operator/(const MPInt &o) const;
  MPInt operator%(const MPInt &o) const;
  MPInt &operator+=(const MPInt &o);
  MPInt &operator-=(const MPInt &o);
  MPInt &operator*=(const MPInt &o);
  MPInt &operator/=(const MPInt &o);
  MPInt &operator%=(const MPInt &o);
  MPInt operator-() const;
  MPInt &operator++();
  MPInt &operator--();

  // Divide by a number that is known to be positive.
  // This is slightly more efficient because it saves an overflow check.
  MPInt divByPositive(const MPInt &o) const;
  MPInt &divByPositiveInPlace(const MPInt &o);

  friend MPInt abs(const MPInt &x);
  friend MPInt gcdRange(ArrayRef<MPInt> range);
  friend MPInt ceilDiv(const MPInt &lhs, const MPInt &rhs);
  friend MPInt floorDiv(const MPInt &lhs, const MPInt &rhs);
  // The operands must be non-negative for gcd.
  friend MPInt gcd(const MPInt &a, const MPInt &b);
  friend MPInt lcm(const MPInt &a, const MPInt &b);
  friend MPInt mod(const MPInt &lhs, const MPInt &rhs);

  std::ostream &print(std::ostream &os) const;
  void dump() const;

  /// ---------------------------------------------------------------------------
  /// Convenience operator overloads for int64_t.
  /// ---------------------------------------------------------------------------
  friend MPInt &operator+=(MPInt &a, int64_t b);
  friend MPInt &operator-=(MPInt &a, int64_t b);
  friend MPInt &operator*=(MPInt &a, int64_t b);
  friend MPInt &operator/=(MPInt &a, int64_t b);
  friend MPInt &operator%=(MPInt &a, int64_t b);

  friend bool operator==(const MPInt &a, int64_t b);
  friend bool operator!=(const MPInt &a, int64_t b);
  friend bool operator>(const MPInt &a, int64_t b);
  friend bool operator<(const MPInt &a, int64_t b);
  friend bool operator<=(const MPInt &a, int64_t b);
  friend bool operator>=(const MPInt &a, int64_t b);
  friend MPInt operator+(const MPInt &a, int64_t b);
  friend MPInt operator-(const MPInt &a, int64_t b);
  friend MPInt operator*(const MPInt &a, int64_t b);
  friend MPInt operator/(const MPInt &a, int64_t b);
  friend MPInt operator%(const MPInt &a, int64_t b);

  friend bool operator==(int64_t a, const MPInt &b);
  friend bool operator!=(int64_t a, const MPInt &b);
  friend bool operator>(int64_t a, const MPInt &b);
  friend bool operator<(int64_t a, const MPInt &b);
  friend bool operator<=(int64_t a, const MPInt &b);
  friend bool operator>=(int64_t a, const MPInt &b);
  friend MPInt operator+(int64_t a, const MPInt &b);
  friend MPInt operator-(int64_t a, const MPInt &b);
  friend MPInt operator*(int64_t a, const MPInt &b);
  friend MPInt operator/(int64_t a, const MPInt &b);
  friend MPInt operator%(int64_t a, const MPInt &b);

  friend hash_code hash_value(const MPInt &x); // NOLINT
};

/// Redeclarations of friend declaration above to
/// make it discoverable by lookups.
hash_code hash_value(const MPInt &x); // NOLINT

/// This just calls through to the operator int64_t, but it's useful when a
/// function pointer is required. (Although this is marked inline, it is still
/// possible to obtain and use a function pointer to this.)
static inline int64_t int64FromMPInt(const MPInt &x) { return int64_t(x); }
FP_ATTRIBUTE_ALWAYS_INLINE MPInt mpintFromInt64(int64_t x) {
  return MPInt(x);
}

std::ostream &operator<<(std::ostream &os, const MPInt &x);

// The RHS is always expected to be positive, and the result
/// is always non-negative.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt mod(const MPInt &lhs, const MPInt &rhs);

namespace detail {
// Division overflows only when trying to negate the minimal signed value.
FP_ATTRIBUTE_ALWAYS_INLINE bool divWouldOverflow(int64_t x, int64_t y) {
  return x == std::numeric_limits<int64_t>::min() && y == -1;
}
} // namespace detail

/// We define the operations here in the header to facilitate inlining.

/// ---------------------------------------------------------------------------
/// Comparison operators.
/// ---------------------------------------------------------------------------
FP_ATTRIBUTE_ALWAYS_INLINE bool MPInt::operator==(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return getSmall() == o.getSmall();
  return detail::SlowMPInt(*this) == detail::SlowMPInt(o);
}
FP_ATTRIBUTE_ALWAYS_INLINE bool MPInt::operator!=(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return getSmall() != o.getSmall();
  return detail::SlowMPInt(*this) != detail::SlowMPInt(o);
}
FP_ATTRIBUTE_ALWAYS_INLINE bool MPInt::operator>(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return getSmall() > o.getSmall();
  return detail::SlowMPInt(*this) > detail::SlowMPInt(o);
}
FP_ATTRIBUTE_ALWAYS_INLINE bool MPInt::operator<(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return getSmall() < o.getSmall();
  return detail::SlowMPInt(*this) < detail::SlowMPInt(o);
}
FP_ATTRIBUTE_ALWAYS_INLINE bool MPInt::operator<=(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return getSmall() <= o.getSmall();
  return detail::SlowMPInt(*this) <= detail::SlowMPInt(o);
}
FP_ATTRIBUTE_ALWAYS_INLINE bool MPInt::operator>=(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return getSmall() >= o.getSmall();
  return detail::SlowMPInt(*this) >= detail::SlowMPInt(o);
}

/// ---------------------------------------------------------------------------
/// Arithmetic operators.
/// ---------------------------------------------------------------------------
FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::operator+(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    MPInt result;
    bool overflow =
        detail::addOverflow(getSmall(), o.getSmall(), result.getSmall());
    if (FP_LIKELY(!overflow))
      return result;
    return MPInt(detail::SlowMPInt(*this) + detail::SlowMPInt(o));
  }
  return MPInt(detail::SlowMPInt(*this) + detail::SlowMPInt(o));
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::operator-(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    MPInt result;
    bool overflow =
        detail::subOverflow(getSmall(), o.getSmall(), result.getSmall());
    if (FP_LIKELY(!overflow))
      return result;
    return MPInt(detail::SlowMPInt(*this) - detail::SlowMPInt(o));
  }
  return MPInt(detail::SlowMPInt(*this) - detail::SlowMPInt(o));
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::operator*(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    MPInt result;
    bool overflow =
        detail::mulOverflow(getSmall(), o.getSmall(), result.getSmall());
    if (FP_LIKELY(!overflow))
      return result;
    return MPInt(detail::SlowMPInt(*this) * detail::SlowMPInt(o));
  }
  return MPInt(detail::SlowMPInt(*this) * detail::SlowMPInt(o));
}

// Division overflows only occur when negating the minimal possible value.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::divByPositive(const MPInt &o) const {
  assert(o > 0);
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return MPInt(getSmall() / o.getSmall());
  return MPInt(detail::SlowMPInt(*this) / detail::SlowMPInt(o));
}

FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::operator/(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    // Division overflows only occur when negating the minimal possible value.
    if (FP_UNLIKELY(detail::divWouldOverflow(getSmall(), o.getSmall())))
      return -*this;
    return MPInt(getSmall() / o.getSmall());
  }
  return MPInt(detail::SlowMPInt(*this) / detail::SlowMPInt(o));
}

FP_ATTRIBUTE_ALWAYS_INLINE MPInt abs(const MPInt &x) {
  return MPInt(x >= 0 ? x : -x);
}
// Division overflows only occur when negating the minimal possible value.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt ceilDiv(const MPInt &lhs, const MPInt &rhs) {
  if (FP_LIKELY(lhs.isSmall() && rhs.isSmall())) {
    if (FP_UNLIKELY(detail::divWouldOverflow(lhs.getSmall(), rhs.getSmall())))
      return -lhs;
    return MPInt(math::ceilDiv(lhs.getSmall(), rhs.getSmall()));
  }
  return MPInt(ceilDiv(detail::SlowMPInt(lhs), detail::SlowMPInt(rhs)));
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt floorDiv(const MPInt &lhs,
                                            const MPInt &rhs) {
  if (FP_LIKELY(lhs.isSmall() && rhs.isSmall())) {
    if (FP_UNLIKELY(detail::divWouldOverflow(lhs.getSmall(), rhs.getSmall())))
      return -lhs;
    return MPInt(math::floorDiv(lhs.getSmall(), rhs.getSmall()));
  }
  return MPInt(floorDiv(detail::SlowMPInt(lhs), detail::SlowMPInt(rhs)));
}
// The RHS is always expected to be positive, and the result
/// is always non-negative.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt mod(const MPInt &lhs, const MPInt &rhs) {
  if (FP_LIKELY(lhs.isSmall() && rhs.isSmall()))
    return MPInt(math::mod(lhs.getSmall(), rhs.getSmall()));
  return MPInt(mod(detail::SlowMPInt(lhs), detail::SlowMPInt(rhs)));
}

FP_ATTRIBUTE_ALWAYS_INLINE MPInt gcd(const MPInt &a, const MPInt &b) {
  assert(a >= 0 && b >= 0 && "operands must be non-negative!");
  if (FP_LIKELY(a.isSmall() && b.isSmall()))
    return MPInt(std::gcd(a.getSmall(), b.getSmall()));
  return MPInt(gcd(detail::SlowMPInt(a), detail::SlowMPInt(b)));
}

/// Returns the least common multiple of 'a' and 'b'.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt lcm(const MPInt &a, const MPInt &b) {
  MPInt x = abs(a);
  MPInt y = abs(b);
  return (x * y) / gcd(x, y);
}

/// This operation cannot overflow.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::operator%(const MPInt &o) const {
  if (FP_LIKELY(isSmall() && o.isSmall()))
    return MPInt(getSmall() % o.getSmall());
  return MPInt(detail::SlowMPInt(*this) % detail::SlowMPInt(o));
}

FP_ATTRIBUTE_ALWAYS_INLINE MPInt MPInt::operator-() const {
  if (FP_LIKELY(isSmall())) {
    if (FP_LIKELY(getSmall() != std::numeric_limits<int64_t>::min()))
      return MPInt(-getSmall());
    return MPInt(-detail::SlowMPInt(*this));
  }
  return MPInt(-detail::SlowMPInt(*this));
}

/// ---------------------------------------------------------------------------
/// Assignment operators, preincrement, predecrement.
/// ---------------------------------------------------------------------------
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator+=(const MPInt &o) {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    int64_t result = getSmall();
    bool overflow = detail::addOverflow(getSmall(), o.getSmall(), result);
    if (FP_LIKELY(!overflow)) {
      getSmall() = result;
      return *this;
    }
    // Note: this return is not strictly required but
    // removing it leads to a performance regression.
    return *this = MPInt(detail::SlowMPInt(*this) + detail::SlowMPInt(o));
  }
  return *this = MPInt(detail::SlowMPInt(*this) + detail::SlowMPInt(o));
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator-=(const MPInt &o) {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    int64_t result = getSmall();
    bool overflow = detail::subOverflow(getSmall(), o.getSmall(), result);
    if (FP_LIKELY(!overflow)) {
      getSmall() = result;
      return *this;
    }
    // Note: this return is not strictly required but
    // removing it leads to a performance regression.
    return *this = MPInt(detail::SlowMPInt(*this) - detail::SlowMPInt(o));
  }
  return *this = MPInt(detail::SlowMPInt(*this) - detail::SlowMPInt(o));
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator*=(const MPInt &o) {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    int64_t result = getSmall();
    bool overflow = detail::mulOverflow(getSmall(), o.getSmall(), result);
    if (FP_LIKELY(!overflow)) {
      getSmall() = result;
      return *this;
    }
    // Note: this return is not strictly required but
    // removing it leads to a performance regression.
    return *this = MPInt(detail::SlowMPInt(*this) * detail::SlowMPInt(o));
  }
  return *this = MPInt(detail::SlowMPInt(*this) * detail::SlowMPInt(o));
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator/=(const MPInt &o) {
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    // Division overflows only occur when negating the minimal possible value.
    if (FP_UNLIKELY(detail::divWouldOverflow(getSmall(), o.getSmall())))
      return *this = -*this;
    getSmall() /= o.getSmall();
    return *this;
  }
  return *this = MPInt(detail::SlowMPInt(*this) / detail::SlowMPInt(o));
}

// Division overflows only occur when the divisor is -1.
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &
MPInt::divByPositiveInPlace(const MPInt &o) {
  assert(o > 0);
  if (FP_LIKELY(isSmall() && o.isSmall())) {
    getSmall() /= o.getSmall();
    return *this;
  }
  return *this = MPInt(detail::SlowMPInt(*this) / detail::SlowMPInt(o));
}

FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator%=(const MPInt &o) {
  return *this = *this % o;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator++() { return *this += 1; }
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &MPInt::operator--() { return *this -= 1; }

/// ----------------------------------------------------------------------------
/// Convenience operator overloads for int64_t.
/// ----------------------------------------------------------------------------
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator+=(MPInt &a, int64_t b) {
  return a = a + b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator-=(MPInt &a, int64_t b) {
  return a = a - b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator*=(MPInt &a, int64_t b) {
  return a = a * b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator/=(MPInt &a, int64_t b) {
  return a = a / b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt &operator%=(MPInt &a, int64_t b) {
  return a = a % b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator+(const MPInt &a, int64_t b) {
  return a + MPInt(b);
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator-(const MPInt &a, int64_t b) {
  return a - MPInt(b);
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator*(const MPInt &a, int64_t b) {
  return a * MPInt(b);
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator/(const MPInt &a, int64_t b) {
  return a / MPInt(b);
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator%(const MPInt &a, int64_t b) {
  return a % MPInt(b);
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator+(int64_t a, const MPInt &b) {
  return MPInt(a) + b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator-(int64_t a, const MPInt &b) {
  return MPInt(a) - b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator*(int64_t a, const MPInt &b) {
  return MPInt(a) * b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator/(int64_t a, const MPInt &b) {
  return MPInt(a) / b;
}
FP_ATTRIBUTE_ALWAYS_INLINE MPInt operator%(int64_t a, const MPInt &b) {
  return MPInt(a) % b;
}

/// We provide special implementations of the comparison operators rather than
/// calling through as above, as this would result in a 1.2x slowdown.
FP_ATTRIBUTE_ALWAYS_INLINE bool operator==(const MPInt &a, int64_t b) {
  if (FP_LIKELY(a.isSmall()))
    return a.getSmall() == b;
  return a.getLarge() == b;
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator!=(const MPInt &a, int64_t b) {
  if (FP_LIKELY(a.isSmall()))
    return a.getSmall() != b;
  return a.getLarge() != b;
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator>(const MPInt &a, int64_t b) {
  if (FP_LIKELY(a.isSmall()))
    return a.getSmall() > b;
  return a.getLarge() > b;
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator<(const MPInt &a, int64_t b) {
  if (FP_LIKELY(a.isSmall()))
    return a.getSmall() < b;
  return a.getLarge() < b;
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator<=(const MPInt &a, int64_t b) {
  if (FP_LIKELY(a.isSmall()))
    return a.getSmall() <= b;
  return a.getLarge() <= b;
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator>=(const MPInt &a, int64_t b) {
  if (FP_LIKELY(a.isSmall()))
    return a.getSmall() >= b;
  return a.getLarge() >= b;
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator==(int64_t a, const MPInt &b) {
  if (FP_LIKELY(b.isSmall()))
    return a == b.getSmall();
  return a == b.getLarge();
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator!=(int64_t a, const MPInt &b) {
  if (FP_LIKELY(b.isSmall()))
    return a != b.getSmall();
  return a != b.getLarge();
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator>(int64_t a, const MPInt &b) {
  if (FP_LIKELY(b.isSmall()))
    return a > b.getSmall();
  return a > b.getLarge();
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator<(int64_t a, const MPInt &b) {
  if (FP_LIKELY(b.isSmall()))
    return a < b.getSmall();
  return a < b.getLarge();
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator<=(int64_t a, const MPInt &b) {
  if (FP_LIKELY(b.isSmall()))
    return a <= b.getSmall();
  return a <= b.getLarge();
}
FP_ATTRIBUTE_ALWAYS_INLINE bool operator>=(int64_t a, const MPInt &b) {
  if (FP_LIKELY(b.isSmall()))
    return a >= b.getSmall();
  return a >= b.getLarge();
}

} // namespace presburger

#endif // FP_MPINT_H

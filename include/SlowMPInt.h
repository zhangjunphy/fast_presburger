//===- SlowMPInt.h - MLIR SlowMPInt Class -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is a simple class to represent arbitrary precision signed integers.
// Unlike APInt, one does not have to specify a fixed maximum size, and the
// integer can take on any arbitrary values.
//
// This class is to be used as a fallback slow path for the MPInt class, and
// is not intended to be used directly.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_ANALYSIS_PRESBURGER_SLOWMPINT_H
#define MLIR_ANALYSIS_PRESBURGER_SLOWMPINT_H

#include <cstdint>
#include <string>

#include <gmp.h>

namespace presburger {
namespace detail {

/// A simple class providing multi-precision arithmetic. Internally, it stores
/// an APInt, whose width is doubled whenever an overflow occurs at a certain
/// width. The default constructor sets the initial width to 64. SlowMPInt is
/// primarily intended to be used as a slow fallback path for the upcoming MPInt
/// class.
class SlowMPInt {
private:
  mpz_t val;

public:
  explicit SlowMPInt(int64_t val);
  SlowMPInt();
  explicit SlowMPInt(const mpz_t &val);
  SlowMPInt(const SlowMPInt& val);
  SlowMPInt &operator=(int64_t val);
  explicit operator int64_t() const;
  SlowMPInt operator-() const;
  bool operator==(const SlowMPInt &o) const;
  bool operator!=(const SlowMPInt &o) const;
  bool operator>(const SlowMPInt &o) const;
  bool operator<(const SlowMPInt &o) const;
  bool operator<=(const SlowMPInt &o) const;
  bool operator>=(const SlowMPInt &o) const;
  SlowMPInt operator+(const SlowMPInt &o) const;
  SlowMPInt operator-(const SlowMPInt &o) const;
  SlowMPInt operator*(const SlowMPInt &o) const;
  SlowMPInt operator/(const SlowMPInt &o) const;
  SlowMPInt operator%(const SlowMPInt &o) const;
  SlowMPInt &operator+=(const SlowMPInt &o);
  SlowMPInt &operator-=(const SlowMPInt &o);
  SlowMPInt &operator*=(const SlowMPInt &o);
  SlowMPInt &operator/=(const SlowMPInt &o);
  SlowMPInt &operator%=(const SlowMPInt &o);

  SlowMPInt &operator++();
  SlowMPInt &operator--();

  friend SlowMPInt abs(const SlowMPInt &x);
  friend SlowMPInt ceilDiv(const SlowMPInt &lhs, const SlowMPInt &rhs);
  friend SlowMPInt floorDiv(const SlowMPInt &lhs, const SlowMPInt &rhs);
  /// The operands must be non-negative for gcd.
  friend SlowMPInt gcd(const SlowMPInt &a, const SlowMPInt &b);
  friend class std::hash<SlowMPInt>;

  void print(std::ostream &os) const;
  void dump() const;

  unsigned getBitWidth() const { return val.getBitWidth(); }
};

std::ostream &operator<<(std::ostream &os, const SlowMPInt &x);

/// Returns the remainder of dividing LHS by RHS.
///
/// The RHS is always expected to be positive, and the result
/// is always non-negative.
SlowMPInt mod(const SlowMPInt &lhs, const SlowMPInt &rhs);

/// Returns the least common multiple of 'a' and 'b'.
SlowMPInt lcm(const SlowMPInt &a, const SlowMPInt &b);

/// Redeclarations of friend declarations above to
/// make it discoverable by lookups.
SlowMPInt abs(const SlowMPInt &x);
SlowMPInt ceilDiv(const SlowMPInt &lhs, const SlowMPInt &rhs);
SlowMPInt floorDiv(const SlowMPInt &lhs, const SlowMPInt &rhs);
SlowMPInt gcd(const SlowMPInt &a, const SlowMPInt &b);

/// ---------------------------------------------------------------------------
/// Convenience operator overloads for int64_t.
/// ---------------------------------------------------------------------------
SlowMPInt &operator+=(SlowMPInt &a, int64_t b);
SlowMPInt &operator-=(SlowMPInt &a, int64_t b);
SlowMPInt &operator*=(SlowMPInt &a, int64_t b);
SlowMPInt &operator/=(SlowMPInt &a, int64_t b);
SlowMPInt &operator%=(SlowMPInt &a, int64_t b);

bool operator==(const SlowMPInt &a, int64_t b);
bool operator!=(const SlowMPInt &a, int64_t b);
bool operator>(const SlowMPInt &a, int64_t b);
bool operator<(const SlowMPInt &a, int64_t b);
bool operator<=(const SlowMPInt &a, int64_t b);
bool operator>=(const SlowMPInt &a, int64_t b);
SlowMPInt operator+(const SlowMPInt &a, int64_t b);
SlowMPInt operator-(const SlowMPInt &a, int64_t b);
SlowMPInt operator*(const SlowMPInt &a, int64_t b);
SlowMPInt operator/(const SlowMPInt &a, int64_t b);
SlowMPInt operator%(const SlowMPInt &a, int64_t b);

bool operator==(int64_t a, const SlowMPInt &b);
bool operator!=(int64_t a, const SlowMPInt &b);
bool operator>(int64_t a, const SlowMPInt &b);
bool operator<(int64_t a, const SlowMPInt &b);
bool operator<=(int64_t a, const SlowMPInt &b);
bool operator>=(int64_t a, const SlowMPInt &b);
SlowMPInt operator+(int64_t a, const SlowMPInt &b);
SlowMPInt operator-(int64_t a, const SlowMPInt &b);
SlowMPInt operator*(int64_t a, const SlowMPInt &b);
SlowMPInt operator/(int64_t a, const SlowMPInt &b);
SlowMPInt operator%(int64_t a, const SlowMPInt &b);
} // namespace detail
} // namespace presburger

template <> struct std::hash<presburger::detail::SlowMPInt> {
  std::size_t
  operator()(const presburger::detail::SlowMPInt &s) const noexcept;
};

#endif // MLIR_ANALYSIS_PRESBURGER_SLOWMPINT_H

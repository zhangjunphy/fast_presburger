//===- SlowMPInt.cpp - MLIR SlowMPInt Class -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SlowMPInt.h"

#include <iostream>
#include <cassert>

using namespace mlir;
using namespace presburger;
using namespace detail;

SlowMPInt::SlowMPInt(int64_t val) {
  mpz_init(this->val);
  mpz_set_si(this->val, val);
}
SlowMPInt::SlowMPInt() : SlowMPInt(0) {}
SlowMPInt::SlowMPInt(const mpz_t &val) {
  mpz_init(this->val);
  mpz_set(this->val, val);
}
SlowMPInt::SlowMPInt(const SlowMPInt &val) { mpz_set(this->val, val.val); }
SlowMPInt &SlowMPInt::operator=(int64_t val) { return *this = SlowMPInt(val); }
SlowMPInt::operator int64_t() const { return mpz_get_si(val); }

std::size_t std::hash<mlir::presburger::detail::SlowMPInt>::operator()(
    const mlir::presburger::detail::SlowMPInt &s) const noexcept {
  mpz_srcptr ptr = static_cast<mpz_srcptr>(s.val);
  std::string_view view{reinterpret_cast<char *>(ptr->_mp_d),
                        abs(ptr->_mp_size) * sizeof(mp_limb_t)};
  size_t result = hash<std::string_view>{}(view);
  if (ptr->_mp_size < 0) {
    result ^= 1;
  }
  return result;
}

/// ---------------------------------------------------------------------------
/// Printing.
/// ---------------------------------------------------------------------------
void SlowMPInt::print(std::ostream &os) const { os << val; }
void SlowMPInt::dump() const { print(std::cerr); }
std::ostream &detail::operator<<(std::ostream &os,
                                      const SlowMPInt &x) {
  x.print(os);
  return os;
}

/// ---------------------------------------------------------------------------
/// Convenience operator overloads for int64_t.
/// ---------------------------------------------------------------------------
SlowMPInt &detail::operator+=(SlowMPInt &a, int64_t b) {
  return a += SlowMPInt(b);
}
SlowMPInt &detail::operator-=(SlowMPInt &a, int64_t b) {
  return a -= SlowMPInt(b);
}
SlowMPInt &detail::operator*=(SlowMPInt &a, int64_t b) {
  return a *= SlowMPInt(b);
}
SlowMPInt &detail::operator/=(SlowMPInt &a, int64_t b) {
  return a /= SlowMPInt(b);
}
SlowMPInt &detail::operator%=(SlowMPInt &a, int64_t b) {
  return a %= SlowMPInt(b);
}

bool detail::operator==(const SlowMPInt &a, int64_t b) {
  return a == SlowMPInt(b);
}
bool detail::operator!=(const SlowMPInt &a, int64_t b) {
  return a != SlowMPInt(b);
}
bool detail::operator>(const SlowMPInt &a, int64_t b) {
  return a > SlowMPInt(b);
}
bool detail::operator<(const SlowMPInt &a, int64_t b) {
  return a < SlowMPInt(b);
}
bool detail::operator<=(const SlowMPInt &a, int64_t b) {
  return a <= SlowMPInt(b);
}
bool detail::operator>=(const SlowMPInt &a, int64_t b) {
  return a >= SlowMPInt(b);
}
SlowMPInt detail::operator+(const SlowMPInt &a, int64_t b) {
  return a + SlowMPInt(b);
}
SlowMPInt detail::operator-(const SlowMPInt &a, int64_t b) {
  return a - SlowMPInt(b);
}
SlowMPInt detail::operator*(const SlowMPInt &a, int64_t b) {
  return a * SlowMPInt(b);
}
SlowMPInt detail::operator/(const SlowMPInt &a, int64_t b) {
  return a / SlowMPInt(b);
}
SlowMPInt detail::operator%(const SlowMPInt &a, int64_t b) {
  return a % SlowMPInt(b);
}

bool detail::operator==(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) == b;
}
bool detail::operator!=(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) != b;
}
bool detail::operator>(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) > b;
}
bool detail::operator<(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) < b;
}
bool detail::operator<=(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) <= b;
}
bool detail::operator>=(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) >= b;
}
SlowMPInt detail::operator+(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) + b;
}
SlowMPInt detail::operator-(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) - b;
}
SlowMPInt detail::operator*(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) * b;
}
SlowMPInt detail::operator/(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) / b;
}
SlowMPInt detail::operator%(int64_t a, const SlowMPInt &b) {
  return SlowMPInt(a) % b;
}

// static unsigned getMaxWidth(const mpz_t &a, const mpz_t &b) {
//   return std::max(a.getBitWidth(), b.getBitWidth());
// }

/// ---------------------------------------------------------------------------
/// Comparison operators.
/// ---------------------------------------------------------------------------

// TODO: consider instead making APInt::compare available and using that.
bool SlowMPInt::operator==(const SlowMPInt &o) const {
  return mpz_cmp(val, o.val) == 0;
}
bool SlowMPInt::operator!=(const SlowMPInt &o) const {
  return mpz_cmp(val, o.val) != 0;
}
bool SlowMPInt::operator>(const SlowMPInt &o) const {
  return mpz_cmp(val, o.val) > 0;
}
bool SlowMPInt::operator<(const SlowMPInt &o) const {
  return mpz_cmp(val, o.val) < 0;
}
bool SlowMPInt::operator<=(const SlowMPInt &o) const {
  return mpz_cmp(val, o.val) <= 0;
}
bool SlowMPInt::operator>=(const SlowMPInt &o) const {
  return mpz_cmp(val, o.val) >= 0;
}

/// ---------------------------------------------------------------------------
/// Arithmetic operators.
/// ---------------------------------------------------------------------------

SlowMPInt SlowMPInt::operator+(const SlowMPInt &o) const {
  SlowMPInt res;
  mpz_add(res.val, val, o.val);
  return res;
}
SlowMPInt SlowMPInt::operator-(const SlowMPInt &o) const {
  SlowMPInt res;
  mpz_sub(res.val, val, o.val);
  return res;
}
SlowMPInt SlowMPInt::operator*(const SlowMPInt &o) const {
  SlowMPInt res;
  mpz_mul(res.val, val, o.val);
  return res;
}
SlowMPInt SlowMPInt::operator/(const SlowMPInt &o) const {
  SlowMPInt res;
  mpz_tdiv_q(res.val, val, o.val);
  return res;
}
SlowMPInt detail::abs(const SlowMPInt &x) { return x >= 0 ? x : -x; }
SlowMPInt detail::ceilDiv(const SlowMPInt &lhs, const SlowMPInt &rhs) {
  if (rhs == -1)
    return -lhs;
  SlowMPInt res;
  mpz_cdiv_q(res.val, lhs.val, rhs.val);
  return res;
}
SlowMPInt detail::floorDiv(const SlowMPInt &lhs, const SlowMPInt &rhs) {
  if (rhs == -1)
    return -lhs;
  SlowMPInt res;
  mpz_fdiv_q(res.val, lhs.val, rhs.val);
  return res;
}
// The RHS is always expected to be positive, and the result
/// is always non-negative.
SlowMPInt detail::mod(const SlowMPInt &lhs, const SlowMPInt &rhs) {
  assert(rhs >= 1 && "mod is only supported for positive divisors!");
  return lhs % rhs < 0 ? lhs % rhs + rhs : lhs % rhs;
}

SlowMPInt detail::gcd(const SlowMPInt &a, const SlowMPInt &b) {
  assert(a >= 0 && b >= 0 && "operands must be non-negative!");
  SlowMPInt res;
  mpz_gcd(res.val, a.val, b.val);
  return res;
}

/// Returns the least common multiple of 'a' and 'b'.
SlowMPInt detail::lcm(const SlowMPInt &a, const SlowMPInt &b) {
  SlowMPInt x = abs(a);
  SlowMPInt y = abs(b);
  return (x * y) / gcd(x, y);
}

/// This operation cannot overflow.
SlowMPInt SlowMPInt::operator%(const SlowMPInt &o) const {
  SlowMPInt res;
  mpz_mod(res.val, val, o.val);
  return res;
}

SlowMPInt SlowMPInt::operator-() const {
  SlowMPInt res;
  mpz_neg(res.val, val);
  return res;
}

/// ---------------------------------------------------------------------------
/// Assignment operators, preincrement, predecrement.
/// ---------------------------------------------------------------------------
SlowMPInt &SlowMPInt::operator+=(const SlowMPInt &o) {
  *this = *this + o;
  return *this;
}
SlowMPInt &SlowMPInt::operator-=(const SlowMPInt &o) {
  *this = *this - o;
  return *this;
}
SlowMPInt &SlowMPInt::operator*=(const SlowMPInt &o) {
  *this = *this * o;
  return *this;
}
SlowMPInt &SlowMPInt::operator/=(const SlowMPInt &o) {
  *this = *this / o;
  return *this;
}
SlowMPInt &SlowMPInt::operator%=(const SlowMPInt &o) {
  *this = *this % o;
  return *this;
}
SlowMPInt &SlowMPInt::operator++() {
  *this += 1;
  return *this;
}

SlowMPInt &SlowMPInt::operator--() {
  *this -= 1;
  return *this;
}

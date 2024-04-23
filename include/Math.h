#pragma once

#include <limits>
#include <type_traits>
#include <cstdint>
#include <cassert>

namespace presburger {
namespace math {
/// Add two signed integers, computing the two's complement truncated result,
/// returning true if overflow occurred.
template <typename T>
std::enable_if_t<std::is_signed<T>::value, T> AddOverflow(T X, T Y, T &Result) {
#if __has_builtin(__builtin_add_overflow)
  return __builtin_add_overflow(X, Y, &Result);
#else
  // Perform the unsigned addition.
  using U = std::make_unsigned_t<T>;
  const U UX = static_cast<U>(X);
  const U UY = static_cast<U>(Y);
  const U UResult = UX + UY;

  // Convert to signed.
  Result = static_cast<T>(UResult);

  // Adding two positive numbers should result in a positive number.
  if (X > 0 && Y > 0)
    return Result <= 0;
  // Adding two negatives should result in a negative number.
  if (X < 0 && Y < 0)
    return Result >= 0;
  return false;
#endif
}

/// Subtract two signed integers, computing the two's complement truncated
/// result, returning true if an overflow ocurred.
template <typename T>
std::enable_if_t<std::is_signed<T>::value, T> SubOverflow(T X, T Y, T &Result) {
#if __has_builtin(__builtin_sub_overflow)
  return __builtin_sub_overflow(X, Y, &Result);
#else
  // Perform the unsigned addition.
  using U = std::make_unsigned_t<T>;
  const U UX = static_cast<U>(X);
  const U UY = static_cast<U>(Y);
  const U UResult = UX - UY;

  // Convert to signed.
  Result = static_cast<T>(UResult);

  // Subtracting a positive number from a negative results in a negative number.
  if (X <= 0 && Y > 0)
    return Result >= 0;
  // Subtracting a negative number from a positive results in a positive number.
  if (X >= 0 && Y < 0)
    return Result <= 0;
  return false;
#endif
}

/// Multiply two signed integers, computing the two's complement truncated
/// result, returning true if an overflow ocurred.
template <typename T>
std::enable_if_t<std::is_signed<T>::value, T> MulOverflow(T X, T Y, T &Result) {
  // Perform the unsigned multiplication on absolute values.
  using U = std::make_unsigned_t<T>;
  const U UX = X < 0 ? (0 - static_cast<U>(X)) : static_cast<U>(X);
  const U UY = Y < 0 ? (0 - static_cast<U>(Y)) : static_cast<U>(Y);
  const U UResult = UX * UY;

  // Convert to signed.
  const bool IsNegative = (X < 0) ^ (Y < 0);
  Result = IsNegative ? (0 - UResult) : UResult;

  // If any of the args was 0, result is 0 and no overflow occurs.
  if (UX == 0 || UY == 0)
    return false;

  // UX and UY are in [1, 2^n], where n is the number of digits.
  // Check how the max allowed absolute value (2^n for negative, 2^(n-1) for
  // positive) divided by an argument compares to the other.
  if (IsNegative)
    return UX > (static_cast<U>(std::numeric_limits<T>::max()) + U(1)) / UY;
  else
    return UX > (static_cast<U>(std::numeric_limits<T>::max())) / UY;
}

/// Returns the result of MLIR's ceildiv operation on constants. The RHS is
/// expected to be non-zero.
inline int64_t ceilDiv(int64_t lhs, int64_t rhs) {
  assert(rhs != 0);
  // C/C++'s integer division rounds towards 0.
  int64_t x = (rhs > 0) ? -1 : 1;
  return ((lhs != 0) && (lhs > 0) == (rhs > 0)) ? ((lhs + x) / rhs) + 1
                                                : -(-lhs / rhs);
}

/// Returns the result of MLIR's floordiv operation on constants. The RHS is
/// expected to be non-zero.
inline int64_t floorDiv(int64_t lhs, int64_t rhs) {
  assert(rhs != 0);
  // C/C++'s integer division rounds towards 0.
  int64_t x = (rhs < 0) ? 1 : -1;
  return ((lhs != 0) && ((lhs < 0) != (rhs < 0))) ? -((-lhs + x) / rhs) - 1
                                                  : lhs / rhs;
}

/// Returns MLIR's mod operation on constants. MLIR's mod operation yields the
/// remainder of the Euclidean division of 'lhs' by 'rhs', and is therefore not
/// C's % operator.  The RHS is always expected to be positive, and the result
/// is always non-negative.
inline int64_t mod(int64_t lhs, int64_t rhs) {
  assert(rhs >= 1);
  return lhs % rhs < 0 ? lhs % rhs + rhs : lhs % rhs;
}
} // namespace math
} // namespace fp

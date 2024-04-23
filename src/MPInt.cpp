//===- MPInt.cpp - MLIR MPInt Class ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MPInt.h"

#include <iostream>

using namespace presburger;

hash_code presburger::hash_value(const MPInt &x) {
  if (x.isSmall())
    return hash_value(x.getSmall());
  return detail::hash_value(x.getLarge());
}

/// ---------------------------------------------------------------------------
/// Printing.
/// ---------------------------------------------------------------------------
std::ostream &MPInt::print(std::ostream &os) const {
  if (isSmall())
    return os << valSmall;
  return os << valLarge;
}

void MPInt::dump() const { print(std::cerr); }

std::ostream &presburger::operator<<(std::ostream &os, const MPInt &x) {
  x.print(os);
  return os;
}

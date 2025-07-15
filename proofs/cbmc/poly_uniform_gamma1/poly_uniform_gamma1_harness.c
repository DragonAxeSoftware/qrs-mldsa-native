// Copyright (c) The mldsa-native project authors
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

#include "poly.h"

void harness(void)
{
  mld_poly *a;
  const uint8_t *seed;
  uint16_t nonce;

  poly_uniform_gamma1(a, seed, nonce);
}

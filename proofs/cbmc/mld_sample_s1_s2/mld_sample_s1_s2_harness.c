// Copyright (c) The mldsa-native project authors
// SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT

#include "sign.h"

static void mld_sample_s1_s2(polyvecl *s1, polyveck *s2,
                             const uint8_t seed[MLDSA_CRHBYTES]);

void harness(void)
{
  polyvecl *s1;
  polyveck *s2;
  uint8_t *seed;

  mld_sample_s1_s2(s1, s2, seed);
}

/*
 * Copyright (c) 2025 The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include "reduce.h"
#include <stdint.h>
#include "params.h"

/*************************************************
 * Name:        montgomery_reduce
 *
 * Description: For finite field element a with -2^{31}MLDSA_Q <= a <=
 *MLDSA_Q*2^31, compute r \equiv a*2^{-32} (mod MLDSA_Q) such that -MLDSA_Q < r
 *< MLDSA_Q.
 *
 * Arguments:   - int64_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t montgomery_reduce(int64_t a)
{
  int32_t t;

  t = (int64_t)(int32_t)a * QINV;
  t = (a - (int64_t)t * MLDSA_Q) >> 32;
  return t;
}

/*************************************************
 * Name:        reduce32
 *
 * Description: For finite field element a with a <= 2^{31} - 2^{22} - 1,
 *              compute r \equiv a (mod MLDSA_Q) such that -6283008 <= r <=
 *6283008.
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t reduce32(int32_t a)
{
  int32_t t;

  t = (a + (1 << 22)) >> 23;
  t = a - t * MLDSA_Q;
  return t;
}

/*************************************************
 * Name:        caddq
 *
 * Description: Add MLDSA_Q if input coefficient is negative.
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t caddq(int32_t a)
{
  a += (a >> 31) & MLDSA_Q;
  return a;
}

/*************************************************
 * Name:        freeze
 *
 * Description: For finite field element a, compute standard
 *              representative r = a mod^+ MLDSA_Q.
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t freeze(int32_t a)
{
  a = reduce32(a);
  a = caddq(a);
  return a;
}

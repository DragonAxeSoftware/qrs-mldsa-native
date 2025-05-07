/*
 * Copyright (c) 2025 The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "ntt.h"
#include "poly.h"
#include "reduce.h"
#include "rounding.h"
#include "symmetric.h"

void poly_reduce(poly *a)
{
  unsigned int i;
  /* TODO: Introduce the following after using inclusive lower bounds in
   * the underlying debug function mld_debug_check_bounds(). */
  /* mld_assert_bound(a->coeffs, MLDSA_N, INT32_MIN, REDUCE_DOMAIN_MAX); */

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(forall(k0, i, MLDSA_N, a->coeffs[k0] == loop_entry(*a).coeffs[k0]))
    invariant(array_bound(a->coeffs, 0, i, -REDUCE_RANGE_MAX, REDUCE_RANGE_MAX)))
  {
    a->coeffs[i] = reduce32(a->coeffs[i]);
  }

  mld_assert_bound(a->coeffs, MLDSA_N, -REDUCE_RANGE_MAX, REDUCE_RANGE_MAX);
}

void poly_caddq(poly *a)
{
  unsigned int i;
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_Q);

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(forall(k0, i, MLDSA_N, a->coeffs[k0] == loop_entry(*a).coeffs[k0]))
    invariant(array_bound(a->coeffs, 0, i, 0, MLDSA_Q))
    )
  {
    a->coeffs[i] = caddq(a->coeffs[i]);
  }

  mld_assert_bound(a->coeffs, MLDSA_N, 0, MLDSA_Q);
}

void poly_add(poly *c, const poly *a, const poly *b)
{
  unsigned int i;

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(forall(k1, 0, i, c->coeffs[k1] == a->coeffs[k1] + b->coeffs[k1])))
  {
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
  }

  cassert(forall(k, 0, MLDSA_N, c->coeffs[k] == a->coeffs[k] + b->coeffs[k]));
}

void poly_sub(poly *c, const poly *a, const poly *b)
{
  unsigned int i;

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(forall(k1, 0, i, c->coeffs[k1] == a->coeffs[k1] - b->coeffs[k1])))
  {
    c->coeffs[i] = a->coeffs[i] - b->coeffs[i];
  }
  cassert(forall(k, 0, MLDSA_N, c->coeffs[k] == a->coeffs[k] - b->coeffs[k]));
}

void poly_shiftl(poly *a)
{
  unsigned int i;
  mld_assert_abs_bound(a->coeffs, MLDSA_N, 1 << (31 - MLDSA_D));

  for (i = 0; i < MLDSA_N; i++)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(forall(k0, i, MLDSA_N, a->coeffs[k0] == loop_entry(*a).coeffs[k0])))
  {
    /* Reference: uses a left shift by MLDSA_D which is undefined behaviour in
     * C90/C99
     */
    a->coeffs[i] *= (1 << MLDSA_D);
  }
}

#if !defined(MLD_USE_NATIVE_NTT)
void poly_ntt(poly *a)
{
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_Q);
  ntt(a->coeffs);
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLD_NTT_BOUND);
}
#else  /* !MLD_USE_NATIVE_NTT */
void poly_ntt(poly *p)
{
  mld_assert_abs_bound(p->coeffs, MLDSA_N, MLDSA_Q);
  mld_ntt_native(p->coeffs);
  mld_assert_abs_bound(p->coeffs, MLDSA_N, MLD_NTT_BOUND);
}
#endif /* MLD_USE_NATIVE_NTT */

#if !defined(MLD_USE_NATIVE_INTT)
void poly_invntt_tomont(poly *a)
{
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_Q);
  invntt_tomont(a->coeffs);
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_Q);
}
#else  /* !MLD_USE_NATIVE_INTT */
void poly_invntt_tomont(poly *a)
{
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_Q);
  mld_intt_native(a->coeffs);
  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_Q);
}
#endif /* MLD_USE_NATIVE_INTT */

void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b)
{
  unsigned int i;

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N))
  {
    c->coeffs[i] = montgomery_reduce((int64_t)a->coeffs[i] * b->coeffs[i]);
  }
}

void poly_power2round(poly *a1, poly *a0, const poly *a)
{
  unsigned int i;
  mld_assert_bound(a->coeffs, MLDSA_N, 0, MLDSA_Q);

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    assigns(i, memory_slice(a0, sizeof(poly)), memory_slice(a1, sizeof(poly)))
    invariant(i <= MLDSA_N)
    invariant(array_bound(a0->coeffs, 0, i, -(MLD_2_POW_D/2)+1, (MLD_2_POW_D/2)+1))
    invariant(array_bound(a1->coeffs, 0, i, 0, (MLD_2_POW_D/2)+1))
  )
  {
    power2round(&a0->coeffs[i], &a1->coeffs[i], a->coeffs[i]);
  }

  mld_assert_bound(a0->coeffs, MLDSA_N, -(MLD_2_POW_D / 2) + 1,
                   (MLD_2_POW_D / 2) + 1);
  mld_assert_bound(a1->coeffs, MLDSA_N, 0, (MLD_2_POW_D / 2) + 1);
}

void poly_decompose(poly *a1, poly *a0, const poly *a)
{
  unsigned int i;
  mld_assert_bound(a->coeffs, MLDSA_N, 0, MLDSA_Q);

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    assigns(i, memory_slice(a0, sizeof(poly)), memory_slice(a1, sizeof(poly)))
    invariant(i <= MLDSA_N)
    invariant(array_bound(a1->coeffs, 0, i, 0, (MLDSA_Q-1)/(2*MLDSA_GAMMA2)))
    invariant(array_abs_bound(a0->coeffs, 0, i, MLDSA_GAMMA2+1))
  )
  {
    decompose(&a0->coeffs[i], &a1->coeffs[i], a->coeffs[i]);
  }

  mld_assert_abs_bound(a0->coeffs, MLDSA_N, MLDSA_GAMMA2 + 1);
  mld_assert_bound(a1->coeffs, MLDSA_N, 0, (MLDSA_Q - 1) / (2 * MLDSA_GAMMA2));
}

unsigned int poly_make_hint(poly *h, const poly *a0, const poly *a1)
{
  unsigned int i, s = 0;

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(s <= i)
  )
  {
    const unsigned int hint_bit = make_hint(a0->coeffs[i], a1->coeffs[i]);
    h->coeffs[i] = hint_bit;
    s += hint_bit;
  }

  mld_assert(s <= MLDSA_N);
  return s;
}

void poly_use_hint(poly *b, const poly *a, const poly *h)
{
  unsigned int i;
  mld_assert_bound(a->coeffs, MLDSA_N, 0, MLDSA_Q);
  mld_assert_bound(h->coeffs, MLDSA_N, 0, 2);

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(array_bound(b->coeffs, 0, i, 0, (MLDSA_Q-1)/(2*MLDSA_GAMMA2)))
  )
  {
    b->coeffs[i] = use_hint(a->coeffs[i], h->coeffs[i]);
  }

  mld_assert_bound(b->coeffs, MLDSA_N, 0, (MLDSA_Q - 1) / (2 * MLDSA_GAMMA2));
}

/* Reference: explicitly checks the bound B to be <= (MLDSA_Q - 1) / 8).
 * This is unnecessary as it's always a compile-time constant.
 * We instead model it as a precondition.
 */
int poly_chknorm(const poly *a, int32_t B)
{
  unsigned int i;
  int rc = 0;
  int32_t t;
  mld_assert_bound(a->coeffs, MLDSA_N, -REDUCE_RANGE_MAX, REDUCE_RANGE_MAX);

  /* It is ok to leak which coefficient violates the bound since
     the probability for each coefficient is independent of secret
     data but we must not leak the sign of the centralized representative. */

  for (i = 0; i < MLDSA_N; ++i)
  __loop__(
    invariant(i <= MLDSA_N)
    invariant(rc == 0 || rc == 1)
    invariant((rc == 0) == array_abs_bound(a->coeffs, 0, i, B))
  )
  {
    /* Absolute value */
    t = a->coeffs[i] >> 31;
    t = a->coeffs[i] - (t & 2 * a->coeffs[i]);

    if (t >= B)
    {
      rc = 1;
    }
  }

  return rc;
}

/*************************************************
 * Name:        rej_uniform
 *
 * Description: Sample uniformly random coefficients in [0, MLDSA_Q-1] by
 *              performing rejection sampling on array of random bytes.
 *
 * Arguments:   - int32_t *a: pointer to output array (allocated)
 *              - unsigned int target:  requested number of coefficients to
 *sample
 *              - unsigned int offset:  number of coefficients already sampled
 *              - const uint8_t *buf: array of random bytes to sample from
 *              - unsigned int buflen: length of array of random bytes (must be
 *                multiple of 3)
 *
 * Returns number of sampled coefficients. Can be smaller than len if not enough
 * random bytes were given.
 **************************************************/

/* Reference: `rej_uniform()` in the reference implementation [@REF].
 *            - Our signature differs from the reference implementation
 *              in that it adds the offset and always expects the base of the
 *              target buffer. This avoids shifting the buffer base in the
 *              caller, which appears tricky to reason about. */
#define POLY_UNIFORM_NBLOCKS \
  ((768 + STREAM128_BLOCKBYTES - 1) / STREAM128_BLOCKBYTES)
static unsigned int rej_uniform(int32_t *a, unsigned int target,
                                unsigned int offset, const uint8_t *buf,
                                unsigned int buflen)
__contract__(
  requires(offset <= target && target <= MLDSA_N)
  requires(buflen <= (POLY_UNIFORM_NBLOCKS * STREAM128_BLOCKBYTES) && buflen % 3 == 0)
  requires(memory_no_alias(a, sizeof(int32_t) * target))
  requires(memory_no_alias(buf, buflen))
  requires(array_bound(a, 0, offset, 0, MLDSA_Q))
  assigns(memory_slice(a, sizeof(int32_t) * target))
  ensures(offset <= return_value && return_value <= target)
  ensures(array_bound(a, 0, return_value, 0, MLDSA_Q))
)
{
  unsigned int ctr, pos;
  uint32_t t;

  ctr = offset;
  pos = 0;
  /* pos + 3 cannot overflow due to the assumption
  buflen <= (POLY_UNIFORM_NBLOCKS * STREAM128_BLOCKBYTES) */
  while (ctr < target && pos + 3 <= buflen)
  __loop__(
    invariant(offset <= ctr && ctr <= target && pos <= buflen)
    invariant(array_bound(a, 0, ctr, 0, MLDSA_Q)))
  {
    t = buf[pos++];
    t |= (uint32_t)buf[pos++] << 8;
    t |= (uint32_t)buf[pos++] << 16;
    t &= 0x7FFFFF;

    if (t < MLDSA_Q)
    {
      a[ctr++] = t;
    }
  }

  return ctr;
}

/* Reference: poly_uniform() in the reference implementation [@REF].
 *           - Simplified from reference by removing buffer tail handling
 *             since buflen % 3 = 0 always holds true (STREAM128_BLOCKBYTES =
 *             168).
 *           - Modified rej_uniform interface to track offset directly. */
void poly_uniform(poly *a, const uint8_t seed[MLDSA_SEEDBYTES], uint16_t nonce)
{
  unsigned int ctr;
  unsigned int buflen = POLY_UNIFORM_NBLOCKS * STREAM128_BLOCKBYTES;
  uint8_t buf[POLY_UNIFORM_NBLOCKS * STREAM128_BLOCKBYTES];
  stream128_state state;

  stream128_init(&state, seed, nonce);
  stream128_squeezeblocks(buf, POLY_UNIFORM_NBLOCKS, &state);

  ctr = rej_uniform(a->coeffs, MLDSA_N, 0, buf, buflen);
  buflen = STREAM128_BLOCKBYTES;
  while (ctr < MLDSA_N)
  __loop__(
    assigns(ctr, state, memory_slice(a, sizeof(poly)), object_whole(buf))
    invariant(ctr <= MLDSA_N)
    invariant(array_bound(a->coeffs, 0, ctr, 0, MLDSA_Q)))
  {
    stream128_squeezeblocks(buf, 1, &state);
    ctr = rej_uniform(a->coeffs, MLDSA_N, ctr, buf, buflen);
  }
}

/*************************************************
 * Name:        rej_eta
 *
 * Description: Sample uniformly random coefficients in [-MLDSA_ETA, MLDSA_ETA]
 *by performing rejection sampling on array of random bytes.
 *
 * Arguments:   - int32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const uint8_t *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len if not enough
 * random bytes were given.
 **************************************************/
#if MLDSA_ETA == 2
#define POLY_UNIFORM_ETA_NBLOCKS \
  ((136 + STREAM256_BLOCKBYTES - 1) / STREAM256_BLOCKBYTES)
#elif MLDSA_ETA == 4
#define POLY_UNIFORM_ETA_NBLOCKS \
  ((227 + STREAM256_BLOCKBYTES - 1) / STREAM256_BLOCKBYTES)
#else
#error "Invalid value of MLDSA_ETA"
#endif
static unsigned int rej_eta(int32_t *a, unsigned int len, const uint8_t *buf,
                            unsigned int buflen)
__contract__(
  requires(len <= buflen && len <= MLDSA_N && \
                  buflen <= (POLY_UNIFORM_ETA_NBLOCKS * STREAM256_BLOCKBYTES))
  requires(memory_no_alias(a, sizeof(int32_t) * len))
  requires(memory_no_alias(buf, buflen))
  assigns(memory_slice(a, sizeof(int32_t) * len))
  ensures(return_value <= len)
  ensures(array_abs_bound(a, 0, return_value, MLDSA_ETA + 1))
)
{
  unsigned int ctr, pos;
  uint32_t t0, t1;

  ctr = pos = 0;
  while (ctr < len && pos < buflen)
  __loop__(
    invariant(0 <= ctr && ctr <= len && pos <= buflen)
    invariant(array_abs_bound(a, 0, ctr, MLDSA_ETA + 1))
  )
  {
    t0 = buf[pos] & 0x0F;
    t1 = buf[pos++] >> 4;

#if MLDSA_ETA == 2
    if (t0 < 15)
    {
      t0 = t0 - (205 * t0 >> 10) * 5;
      a[ctr++] = 2 - (int32_t)t0;
    }
    if (t1 < 15 && ctr < len)
    {
      t1 = t1 - (205 * t1 >> 10) * 5;
      a[ctr++] = 2 - (int32_t)t1;
    }
#elif MLDSA_ETA == 4
    if (t0 < 9)
    {
      a[ctr++] = 4 - (int32_t)t0;
    }
    if (t1 < 9 && ctr < len)
    {
      a[ctr++] = 4 - (int32_t)t1;
    }
#else /* MLDSA_ETA == 4 */
#error "Invalid value of MLDSA_ETA"
#endif /* MLDSA_ETA != 2 && MLDSA_ETA != 4 */
  }

  return ctr;
}


void poly_uniform_eta(poly *a, const uint8_t seed[MLDSA_CRHBYTES],
                      uint16_t nonce)
{
  unsigned int ctr;
  unsigned int buflen = POLY_UNIFORM_ETA_NBLOCKS * STREAM256_BLOCKBYTES;
  uint8_t buf[POLY_UNIFORM_ETA_NBLOCKS * STREAM256_BLOCKBYTES];
  stream256_state state;

  stream256_init(&state, seed, nonce);
  stream256_squeezeblocks(buf, POLY_UNIFORM_ETA_NBLOCKS, &state);

  ctr = rej_eta(a->coeffs, MLDSA_N, buf, buflen);

  while (ctr < MLDSA_N)
  {
    stream256_squeezeblocks(buf, 1, &state);
    ctr += rej_eta(a->coeffs + ctr, MLDSA_N - ctr, buf, STREAM256_BLOCKBYTES);
  }
}

#define POLY_UNIFORM_GAMMA1_NBLOCKS \
  ((MLDSA_POLYZ_PACKEDBYTES + STREAM256_BLOCKBYTES - 1) / STREAM256_BLOCKBYTES)
void poly_uniform_gamma1(poly *a, const uint8_t seed[MLDSA_CRHBYTES],
                         uint16_t nonce)
{
  uint8_t buf[POLY_UNIFORM_GAMMA1_NBLOCKS * STREAM256_BLOCKBYTES];
  stream256_state state;

  stream256_init(&state, seed, nonce);
  stream256_squeezeblocks(buf, POLY_UNIFORM_GAMMA1_NBLOCKS, &state);
  polyz_unpack(a, buf);
}

void poly_challenge(poly *c, const uint8_t seed[MLDSA_CTILDEBYTES])
{
  unsigned int i, j, pos;
  uint64_t signs;
  uint64_t offset;
  uint8_t buf[SHAKE256_RATE];
  keccak_state state;

  shake256_init(&state);
  shake256_absorb(&state, seed, MLDSA_CTILDEBYTES);
  shake256_finalize(&state);
  shake256_squeezeblocks(buf, 1, &state);

  /* Convert the first 8 bytes of buf[] into an unsigned 64-bit value.   */
  /* Each bit of that dictates the sign of the resulting challenge value */
  signs = 0;
  for (i = 0; i < 8; ++i)
  __loop__(
    assigns(i, signs)
    invariant(i <= 8)
  )
  {
    signs |= (uint64_t)buf[i] << 8 * i;
  }
  pos = 8;

  memset(c, 0, sizeof(poly));

  for (i = MLDSA_N - MLDSA_TAU; i < MLDSA_N; ++i)
  __loop__(
    assigns(i, j, object_whole(buf), state, pos, memory_slice(c, sizeof(poly)), signs)
    invariant(i >= MLDSA_N - MLDSA_TAU)
    invariant(i <= MLDSA_N)
    invariant(pos >= 1)
    invariant(pos <= SHAKE256_RATE)
    invariant(array_bound(c->coeffs, 0, MLDSA_N, -1, 2))
  )
  {
    do
    __loop__(
      assigns(j, object_whole(buf), state, pos)
    )
    {
      if (pos >= SHAKE256_RATE)
      {
        shake256_squeezeblocks(buf, 1, &state);
        pos = 0;
      }
      j = buf[pos++];
    } while (j > i);

    c->coeffs[i] = c->coeffs[j];

    /* Reference: Compute coefficent value here in two steps to */
    /* mixinf unsigned and signed arithmetic with implicit      */
    /* conversions, and so that CBMC can keep track of ranges   */
    /* to complete type-safety proof here.                      */

    /* The least-significant bit of signs tells us if we want -1 or +1 */
    offset = 2 * (signs & 1);

    /* offset has value 0 or 2 here, so (1 - (int32_t) offset) has
     * value -1 or +1 */
    c->coeffs[j] = 1 - (int32_t)offset;

    /* Move to the next bit of signs for next time */
    signs >>= 1;
  }

  mld_assert_bound(c->coeffs, MLDSA_N, -1, 2);
}

void polyeta_pack(uint8_t *r, const poly *a)
{
  unsigned int i;
  uint8_t t[8];

  mld_assert_abs_bound(a->coeffs, MLDSA_N, MLDSA_ETA + 1);

#if MLDSA_ETA == 2
  for (i = 0; i < MLDSA_N / 8; ++i)
  __loop__(
    invariant(i <= MLDSA_N/8))
  {
    t[0] = MLDSA_ETA - a->coeffs[8 * i + 0];
    t[1] = MLDSA_ETA - a->coeffs[8 * i + 1];
    t[2] = MLDSA_ETA - a->coeffs[8 * i + 2];
    t[3] = MLDSA_ETA - a->coeffs[8 * i + 3];
    t[4] = MLDSA_ETA - a->coeffs[8 * i + 4];
    t[5] = MLDSA_ETA - a->coeffs[8 * i + 5];
    t[6] = MLDSA_ETA - a->coeffs[8 * i + 6];
    t[7] = MLDSA_ETA - a->coeffs[8 * i + 7];

    r[3 * i + 0] = ((t[0] >> 0) | (t[1] << 3) | (t[2] << 6)) & 0xFF;
    r[3 * i + 1] =
        ((t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7)) & 0xFF;
    r[3 * i + 2] = ((t[5] >> 1) | (t[6] << 2) | (t[7] << 5)) & 0xFF;
  }
#elif MLDSA_ETA == 4
  for (i = 0; i < MLDSA_N / 2; ++i)
  __loop__(
    invariant(i <= MLDSA_N/2))
  {
    t[0] = MLDSA_ETA - a->coeffs[2 * i + 0];
    t[1] = MLDSA_ETA - a->coeffs[2 * i + 1];
    r[i] = t[0] | (t[1] << 4);
  }
#else /* MLDSA_ETA == 4 */
#error "Invalid value of MLDSA_ETA"
#endif /* MLDSA_ETA != 2 && MLDSA_ETA != 4 */
}

void polyeta_unpack(poly *r, const uint8_t *a)
{
  unsigned int i;

#if MLDSA_ETA == 2
  for (i = 0; i < MLDSA_N / 8; ++i)
  __loop__(
    invariant(i <= MLDSA_N/8)
    invariant(array_bound(r->coeffs, 0, i*8, -5, MLDSA_ETA + 1)))
  {
    r->coeffs[8 * i + 0] = (a[3 * i + 0] >> 0) & 7;
    r->coeffs[8 * i + 1] = (a[3 * i + 0] >> 3) & 7;
    r->coeffs[8 * i + 2] = ((a[3 * i + 0] >> 6) | (a[3 * i + 1] << 2)) & 7;
    r->coeffs[8 * i + 3] = (a[3 * i + 1] >> 1) & 7;
    r->coeffs[8 * i + 4] = (a[3 * i + 1] >> 4) & 7;
    r->coeffs[8 * i + 5] = ((a[3 * i + 1] >> 7) | (a[3 * i + 2] << 1)) & 7;
    r->coeffs[8 * i + 6] = (a[3 * i + 2] >> 2) & 7;
    r->coeffs[8 * i + 7] = (a[3 * i + 2] >> 5) & 7;

    r->coeffs[8 * i + 0] = MLDSA_ETA - r->coeffs[8 * i + 0];
    r->coeffs[8 * i + 1] = MLDSA_ETA - r->coeffs[8 * i + 1];
    r->coeffs[8 * i + 2] = MLDSA_ETA - r->coeffs[8 * i + 2];
    r->coeffs[8 * i + 3] = MLDSA_ETA - r->coeffs[8 * i + 3];
    r->coeffs[8 * i + 4] = MLDSA_ETA - r->coeffs[8 * i + 4];
    r->coeffs[8 * i + 5] = MLDSA_ETA - r->coeffs[8 * i + 5];
    r->coeffs[8 * i + 6] = MLDSA_ETA - r->coeffs[8 * i + 6];
    r->coeffs[8 * i + 7] = MLDSA_ETA - r->coeffs[8 * i + 7];
  }
#elif MLDSA_ETA == 4
  for (i = 0; i < MLDSA_N / 2; ++i)
  __loop__(
    invariant(i <= MLDSA_N/2)
    invariant(array_bound(r->coeffs, 0, i*2, -11, MLDSA_ETA + 1)))
  {
    r->coeffs[2 * i + 0] = a[i] & 0x0F;
    r->coeffs[2 * i + 1] = a[i] >> 4;
    r->coeffs[2 * i + 0] = MLDSA_ETA - r->coeffs[2 * i + 0];
    r->coeffs[2 * i + 1] = MLDSA_ETA - r->coeffs[2 * i + 1];
  }
#else /* MLDSA_ETA == 4 */
#error "Invalid value of MLDSA_ETA"
#endif /* MLDSA_ETA != 2 && MLDSA_ETA != 4 */

  mld_assert_bound(r->coeffs, MLDSA_N, MLD_POLYETA_UNPACK_LOWER_BOUND,
                   MLDSA_ETA + 1);
}

void polyt1_pack(uint8_t *r, const poly *a)
{
  unsigned int i;
  mld_assert_bound(a->coeffs, MLDSA_N, 0, 1 << 10);

  for (i = 0; i < MLDSA_N / 4; ++i)
  __loop__(
    invariant(i <= MLDSA_N/4))
  {
    r[5 * i + 0] = (a->coeffs[4 * i + 0] >> 0) & 0xFF;
    r[5 * i + 1] =
        ((a->coeffs[4 * i + 0] >> 8) | (a->coeffs[4 * i + 1] << 2)) & 0xFF;
    r[5 * i + 2] =
        ((a->coeffs[4 * i + 1] >> 6) | (a->coeffs[4 * i + 2] << 4)) & 0xFF;
    r[5 * i + 3] =
        ((a->coeffs[4 * i + 2] >> 4) | (a->coeffs[4 * i + 3] << 6)) & 0xFF;
    r[5 * i + 4] = (a->coeffs[4 * i + 3] >> 2) & 0xFF;
  }
}

void polyt1_unpack(poly *r, const uint8_t *a)
{
  unsigned int i;

  for (i = 0; i < MLDSA_N / 4; ++i)
  __loop__(
    invariant(i <= MLDSA_N/4)
    invariant(array_bound(r->coeffs, 0, i*4, 0, 1 << 10)))
  {
    r->coeffs[4 * i + 0] =
        ((a[5 * i + 0] >> 0) | ((uint32_t)a[5 * i + 1] << 8)) & 0x3FF;
    r->coeffs[4 * i + 1] =
        ((a[5 * i + 1] >> 2) | ((uint32_t)a[5 * i + 2] << 6)) & 0x3FF;
    r->coeffs[4 * i + 2] =
        ((a[5 * i + 2] >> 4) | ((uint32_t)a[5 * i + 3] << 4)) & 0x3FF;
    r->coeffs[4 * i + 3] =
        ((a[5 * i + 3] >> 6) | ((uint32_t)a[5 * i + 4] << 2)) & 0x3FF;
  }

  mld_assert_bound(r->coeffs, MLDSA_N, 0, 1 << 10);
}

void polyt0_pack(uint8_t *r, const poly *a)
{
  unsigned int i;
  uint32_t t[8];

  mld_assert_bound(a->coeffs, MLDSA_N, -(1 << (MLDSA_D - 1)) + 1,
                   (1 << (MLDSA_D - 1)) + 1);

  for (i = 0; i < MLDSA_N / 8; ++i)
  __loop__(
    invariant(i <= MLDSA_N/8))
  {
    t[0] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 0];
    t[1] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 1];
    t[2] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 2];
    t[3] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 3];
    t[4] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 4];
    t[5] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 5];
    t[6] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 6];
    t[7] = (1 << (MLDSA_D - 1)) - a->coeffs[8 * i + 7];

    r[13 * i + 0] = (t[0]) & 0xFF;
    r[13 * i + 1] = (t[0] >> 8) & 0xFF;
    r[13 * i + 1] |= (t[1] << 5) & 0xFF;
    r[13 * i + 2] = (t[1] >> 3) & 0xFF;
    r[13 * i + 3] = (t[1] >> 11) & 0xFF;
    r[13 * i + 3] |= (t[2] << 2) & 0xFF;
    r[13 * i + 4] = (t[2] >> 6) & 0xFF;
    r[13 * i + 4] |= (t[3] << 7) & 0xFF;
    r[13 * i + 5] = (t[3] >> 1) & 0xFF;
    r[13 * i + 6] = (t[3] >> 9) & 0xFF;
    r[13 * i + 6] |= (t[4] << 4) & 0xFF;
    r[13 * i + 7] = (t[4] >> 4) & 0xFF;
    r[13 * i + 8] = (t[4] >> 12) & 0xFF;
    r[13 * i + 8] |= (t[5] << 1) & 0xFF;
    r[13 * i + 9] = (t[5] >> 7) & 0xFF;
    r[13 * i + 9] |= (t[6] << 6) & 0xFF;
    r[13 * i + 10] = (t[6] >> 2) & 0xFF;
    r[13 * i + 11] = (t[6] >> 10) & 0xFF;
    r[13 * i + 11] |= (t[7] << 3) & 0xFF;
    r[13 * i + 12] = (t[7] >> 5) & 0xFF;
  }
}

void polyt0_unpack(poly *r, const uint8_t *a)
{
  unsigned int i;

  for (i = 0; i < MLDSA_N / 8; ++i)
  __loop__(
    invariant(i <= MLDSA_N/8)
    invariant(array_bound(r->coeffs, 0, i*8, -(1<<(MLDSA_D-1)) + 1, (1<<(MLDSA_D-1)) + 1)))
  {
    r->coeffs[8 * i + 0] = a[13 * i + 0];
    r->coeffs[8 * i + 0] |= (uint32_t)a[13 * i + 1] << 8;
    r->coeffs[8 * i + 0] &= 0x1FFF;

    r->coeffs[8 * i + 1] = a[13 * i + 1] >> 5;
    r->coeffs[8 * i + 1] |= (uint32_t)a[13 * i + 2] << 3;
    r->coeffs[8 * i + 1] |= (uint32_t)a[13 * i + 3] << 11;
    r->coeffs[8 * i + 1] &= 0x1FFF;

    r->coeffs[8 * i + 2] = a[13 * i + 3] >> 2;
    r->coeffs[8 * i + 2] |= (uint32_t)a[13 * i + 4] << 6;
    r->coeffs[8 * i + 2] &= 0x1FFF;

    r->coeffs[8 * i + 3] = a[13 * i + 4] >> 7;
    r->coeffs[8 * i + 3] |= (uint32_t)a[13 * i + 5] << 1;
    r->coeffs[8 * i + 3] |= (uint32_t)a[13 * i + 6] << 9;
    r->coeffs[8 * i + 3] &= 0x1FFF;

    r->coeffs[8 * i + 4] = a[13 * i + 6] >> 4;
    r->coeffs[8 * i + 4] |= (uint32_t)a[13 * i + 7] << 4;
    r->coeffs[8 * i + 4] |= (uint32_t)a[13 * i + 8] << 12;
    r->coeffs[8 * i + 4] &= 0x1FFF;

    r->coeffs[8 * i + 5] = a[13 * i + 8] >> 1;
    r->coeffs[8 * i + 5] |= (uint32_t)a[13 * i + 9] << 7;
    r->coeffs[8 * i + 5] &= 0x1FFF;

    r->coeffs[8 * i + 6] = a[13 * i + 9] >> 6;
    r->coeffs[8 * i + 6] |= (uint32_t)a[13 * i + 10] << 2;
    r->coeffs[8 * i + 6] |= (uint32_t)a[13 * i + 11] << 10;
    r->coeffs[8 * i + 6] &= 0x1FFF;

    r->coeffs[8 * i + 7] = a[13 * i + 11] >> 3;
    r->coeffs[8 * i + 7] |= (uint32_t)a[13 * i + 12] << 5;
    r->coeffs[8 * i + 7] &= 0x1FFF;

    r->coeffs[8 * i + 0] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 0];
    r->coeffs[8 * i + 1] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 1];
    r->coeffs[8 * i + 2] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 2];
    r->coeffs[8 * i + 3] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 3];
    r->coeffs[8 * i + 4] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 4];
    r->coeffs[8 * i + 5] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 5];
    r->coeffs[8 * i + 6] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 6];
    r->coeffs[8 * i + 7] = (1 << (MLDSA_D - 1)) - r->coeffs[8 * i + 7];
  }

  mld_assert_bound(r->coeffs, MLDSA_N, -(1 << (MLDSA_D - 1)) + 1,
                   (1 << (MLDSA_D - 1)) + 1);
}

void polyz_pack(uint8_t *r, const poly *a)
{
  unsigned int i;
  uint32_t t[4];

  mld_assert_bound(a->coeffs, MLDSA_N, -(MLDSA_GAMMA1 - 1), MLDSA_GAMMA1 + 1);

#if MLDSA_MODE == 2
  for (i = 0; i < MLDSA_N / 4; ++i)
  __loop__(
    invariant(i <= MLDSA_N/4))
  {
    t[0] = MLDSA_GAMMA1 - a->coeffs[4 * i + 0];
    t[1] = MLDSA_GAMMA1 - a->coeffs[4 * i + 1];
    t[2] = MLDSA_GAMMA1 - a->coeffs[4 * i + 2];
    t[3] = MLDSA_GAMMA1 - a->coeffs[4 * i + 3];

    r[9 * i + 0] = (t[0]) & 0xFF;
    r[9 * i + 1] = (t[0] >> 8) & 0xFF;
    r[9 * i + 2] = (t[0] >> 16) & 0xFF;
    r[9 * i + 2] |= (t[1] << 2) & 0xFF;
    r[9 * i + 3] = (t[1] >> 6) & 0xFF;
    r[9 * i + 4] = (t[1] >> 14) & 0xFF;
    r[9 * i + 4] |= (t[2] << 4) & 0xFF;
    r[9 * i + 5] = (t[2] >> 4) & 0xFF;
    r[9 * i + 6] = (t[2] >> 12) & 0xFF;
    r[9 * i + 6] |= (t[3] << 6) & 0xFF;
    r[9 * i + 7] = (t[3] >> 2) & 0xFF;
    r[9 * i + 8] = (t[3] >> 10) & 0xFF;
  }
#else  /* MLDSA_MODE == 2 */
  for (i = 0; i < MLDSA_N / 2; ++i)
  __loop__(
    invariant(i <= MLDSA_N/2))
  {
    t[0] = MLDSA_GAMMA1 - a->coeffs[2 * i + 0];
    t[1] = MLDSA_GAMMA1 - a->coeffs[2 * i + 1];

    r[5 * i + 0] = (t[0]) & 0xFF;
    r[5 * i + 1] = (t[0] >> 8) & 0xFF;
    r[5 * i + 2] = (t[0] >> 16) & 0xFF;
    r[5 * i + 2] |= (t[1] << 4) & 0xFF;
    r[5 * i + 3] = (t[1] >> 4) & 0xFF;
    r[5 * i + 4] = (t[1] >> 12) & 0xFF;
  }
#endif /* MLDSA_MODE != 2 */
}

void polyz_unpack(poly *r, const uint8_t *a)
{
  unsigned int i;

#if MLDSA_MODE == 2
  for (i = 0; i < MLDSA_N / 4; ++i)
  __loop__(
    invariant(i <= MLDSA_N/4)
    invariant(array_bound(r->coeffs, 0, i*4, -(MLDSA_GAMMA1 - 1), MLDSA_GAMMA1 + 1)))
  {
    r->coeffs[4 * i + 0] = a[9 * i + 0];
    r->coeffs[4 * i + 0] |= (uint32_t)a[9 * i + 1] << 8;
    r->coeffs[4 * i + 0] |= (uint32_t)a[9 * i + 2] << 16;
    r->coeffs[4 * i + 0] &= 0x3FFFF;

    r->coeffs[4 * i + 1] = a[9 * i + 2] >> 2;
    r->coeffs[4 * i + 1] |= (uint32_t)a[9 * i + 3] << 6;
    r->coeffs[4 * i + 1] |= (uint32_t)a[9 * i + 4] << 14;
    r->coeffs[4 * i + 1] &= 0x3FFFF;

    r->coeffs[4 * i + 2] = a[9 * i + 4] >> 4;
    r->coeffs[4 * i + 2] |= (uint32_t)a[9 * i + 5] << 4;
    r->coeffs[4 * i + 2] |= (uint32_t)a[9 * i + 6] << 12;
    r->coeffs[4 * i + 2] &= 0x3FFFF;

    r->coeffs[4 * i + 3] = a[9 * i + 6] >> 6;
    r->coeffs[4 * i + 3] |= (uint32_t)a[9 * i + 7] << 2;
    r->coeffs[4 * i + 3] |= (uint32_t)a[9 * i + 8] << 10;
    r->coeffs[4 * i + 3] &= 0x3FFFF;

    r->coeffs[4 * i + 0] = MLDSA_GAMMA1 - r->coeffs[4 * i + 0];
    r->coeffs[4 * i + 1] = MLDSA_GAMMA1 - r->coeffs[4 * i + 1];
    r->coeffs[4 * i + 2] = MLDSA_GAMMA1 - r->coeffs[4 * i + 2];
    r->coeffs[4 * i + 3] = MLDSA_GAMMA1 - r->coeffs[4 * i + 3];
  }
#else  /* MLDSA_MODE == 2 */
  for (i = 0; i < MLDSA_N / 2; ++i)
  __loop__(
    invariant(i <= MLDSA_N/2)
    invariant(array_bound(r->coeffs, 0, i*2, -(MLDSA_GAMMA1 - 1), MLDSA_GAMMA1 + 1)))
  {
    r->coeffs[2 * i + 0] = a[5 * i + 0];
    r->coeffs[2 * i + 0] |= (uint32_t)a[5 * i + 1] << 8;
    r->coeffs[2 * i + 0] |= (uint32_t)a[5 * i + 2] << 16;
    r->coeffs[2 * i + 0] &= 0xFFFFF;

    r->coeffs[2 * i + 1] = a[5 * i + 2] >> 4;
    r->coeffs[2 * i + 1] |= (uint32_t)a[5 * i + 3] << 4;
    r->coeffs[2 * i + 1] |= (uint32_t)a[5 * i + 4] << 12;
    /* r->coeffs[2*i+1] &= 0xFFFFF; */ /* No effect, since we're anyway at 20
                                          bits */

    r->coeffs[2 * i + 0] = MLDSA_GAMMA1 - r->coeffs[2 * i + 0];
    r->coeffs[2 * i + 1] = MLDSA_GAMMA1 - r->coeffs[2 * i + 1];
  }
#endif /* MLDSA_MODE != 2 */

  mld_assert_bound(r->coeffs, MLDSA_N, -(MLDSA_GAMMA1 - 1), MLDSA_GAMMA1 + 1);
}

void polyw1_pack(uint8_t *r, const poly *a)
{
  unsigned int i;

#if MLDSA_MODE == 2
  mld_assert_bound(a->coeffs, MLDSA_N, 0, 44);

  for (i = 0; i < MLDSA_N / 4; ++i)
  __loop__(
    invariant(i <= MLDSA_N/4))
  {
    r[3 * i + 0] = (a->coeffs[4 * i + 0]) & 0xFF;
    r[3 * i + 0] |= (a->coeffs[4 * i + 1] << 6) & 0xFF;
    r[3 * i + 1] = (a->coeffs[4 * i + 1] >> 2) & 0xFF;
    r[3 * i + 1] |= (a->coeffs[4 * i + 2] << 4) & 0xFF;
    r[3 * i + 2] = (a->coeffs[4 * i + 2] >> 4) & 0xFF;
    r[3 * i + 2] |= (a->coeffs[4 * i + 3] << 2) & 0xFF;
  }
#else  /* MLDSA_MODE == 2 */
  mld_assert_bound(a->coeffs, MLDSA_N, 0, 16);

  for (i = 0; i < MLDSA_N / 2; ++i)
  __loop__(
    invariant(i <= MLDSA_N/2))
  {
    r[i] = a->coeffs[2 * i + 0] | (a->coeffs[2 * i + 1] << 4);
  }
#endif /* MLDSA_MODE != 2 */
}

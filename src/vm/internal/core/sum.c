/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/sum.h"

#if defined(_M_X64) || defined(__x86_64__) || defined(__SSE2__)
#include <immintrin.h>
#define GRAPHION_SSE2_SUMS 1
#endif

#if defined(__AVX2__)
#define GRAPHION_AVX2_SUMS 1
#endif

uint64_t sum_weight_slice_wrap(const int64_t *values, size_t count) {
#if defined(GRAPHION_AVX2_SUMS)
  __m256i acc0 = _mm256_setzero_si256();
  __m256i acc1 = _mm256_setzero_si256();
  uint64_t lanes[4] = {0U, 0U, 0U, 0U};
  size_t i = 0U;

  for (; i + 8U <= count; i += 8U) {
    const __m256i chunk0 = _mm256_loadu_si256((const __m256i *)(const void *)(values + i));
    const __m256i chunk1 = _mm256_loadu_si256((const __m256i *)(const void *)(values + i + 4U));
    acc0 = _mm256_add_epi64(acc0, chunk0);
    acc1 = _mm256_add_epi64(acc1, chunk1);
  }
  acc0 = _mm256_add_epi64(acc0, acc1);
  for (; i + 4U <= count; i += 4U) {
    const __m256i chunk = _mm256_loadu_si256((const __m256i *)(const void *)(values + i));
    acc0 = _mm256_add_epi64(acc0, chunk);
  }
  _mm256_storeu_si256((__m256i *)(void *)lanes, acc0);
  {
    uint64_t sum = (lanes[0] + lanes[1]) + (lanes[2] + lanes[3]);
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#elif defined(GRAPHION_SSE2_SUMS)
  __m128i acc = _mm_setzero_si128();
  uint64_t lanes[2] = {0U, 0U};
  size_t i = 0U;

  for (; i + 2U <= count; i += 2U) {
    const __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)(values + i));
    acc = _mm_add_epi64(acc, chunk);
  }
  _mm_storeu_si128((__m128i *)(void *)lanes, acc);
  {
    uint64_t sum = lanes[0] + lanes[1];
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#else
  uint64_t sum0 = 0U;
  uint64_t sum1 = 0U;
  uint64_t sum2 = 0U;
  uint64_t sum3 = 0U;
  size_t i = 0U;

  for (; i + 4U <= count; i += 4U) {
    sum0 += (uint64_t)values[i];
    sum1 += (uint64_t)values[i + 1U];
    sum2 += (uint64_t)values[i + 2U];
    sum3 += (uint64_t)values[i + 3U];
  }
  for (; i < count; ++i) {
    sum0 += (uint64_t)values[i];
  }
  return (sum0 + sum1) + (sum2 + sum3);
#endif
}

uint64_t sum_attr_slice_wrap(const uint32_t *values, size_t count) {
#if defined(GRAPHION_AVX2_SUMS)
  __m256i acc0 = _mm256_setzero_si256();
  __m256i acc1 = _mm256_setzero_si256();
  uint64_t lanes[4] = {0U, 0U, 0U, 0U};
  size_t i = 0U;

  for (; i + 8U <= count; i += 8U) {
    const __m256i chunk = _mm256_loadu_si256((const __m256i *)(const void *)(values + i));
    const __m128i lo = _mm256_castsi256_si128(chunk);
    const __m128i hi = _mm256_extracti128_si256(chunk, 1);
    acc0 = _mm256_add_epi64(acc0, _mm256_cvtepu32_epi64(lo));
    acc1 = _mm256_add_epi64(acc1, _mm256_cvtepu32_epi64(hi));
  }
  acc0 = _mm256_add_epi64(acc0, acc1);
  for (; i + 4U <= count; i += 4U) {
    const __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)(values + i));
    acc0 = _mm256_add_epi64(acc0, _mm256_cvtepu32_epi64(chunk));
  }
  _mm256_storeu_si256((__m256i *)(void *)lanes, acc0);
  {
    uint64_t sum = (lanes[0] + lanes[1]) + (lanes[2] + lanes[3]);
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#elif defined(GRAPHION_SSE2_SUMS)
  const __m128i zero = _mm_setzero_si128();
  __m128i acc_lo = _mm_setzero_si128();
  __m128i acc_hi = _mm_setzero_si128();
  uint64_t lanes_lo[2] = {0U, 0U};
  uint64_t lanes_hi[2] = {0U, 0U};
  size_t i = 0U;

  for (; i + 4U <= count; i += 4U) {
    const __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)(values + i));
    acc_lo = _mm_add_epi64(acc_lo, _mm_unpacklo_epi32(chunk, zero));
    acc_hi = _mm_add_epi64(acc_hi, _mm_unpackhi_epi32(chunk, zero));
  }
  _mm_storeu_si128((__m128i *)(void *)lanes_lo, acc_lo);
  _mm_storeu_si128((__m128i *)(void *)lanes_hi, acc_hi);
  {
    uint64_t sum = (lanes_lo[0] + lanes_lo[1]) + (lanes_hi[0] + lanes_hi[1]);
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#else
  uint64_t sum0 = 0U;
  uint64_t sum1 = 0U;
  uint64_t sum2 = 0U;
  uint64_t sum3 = 0U;
  size_t i = 0U;

  for (; i + 4U <= count; i += 4U) {
    sum0 += (uint64_t)values[i];
    sum1 += (uint64_t)values[i + 1U];
    sum2 += (uint64_t)values[i + 2U];
    sum3 += (uint64_t)values[i + 3U];
  }
  for (; i < count; ++i) {
    sum0 += (uint64_t)values[i];
  }
  return (sum0 + sum1) + (sum2 + sum3);
#endif
}


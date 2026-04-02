/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_SUM_H
#define GRAPHION_VM_SUM_H

#include <stddef.h>
#include <stdint.h>

uint64_t sum_weight_slice_wrap(const int64_t *values, size_t count);
uint64_t sum_attr_slice_wrap(const uint32_t *values, size_t count);

#endif


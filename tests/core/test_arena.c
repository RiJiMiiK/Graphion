/* SPDX-License-Identifier: MIT */

#include "runtime/arena.h"

#include <stdint.h>

int test_arena_alignment_and_reset(void) {
  uint8_t backing[64];
  graphion_arena arena;
  void *p1;
  void *p2;

  if (graphion_arena_init(&arena, backing, sizeof(backing)) != 0) {
    return 1;
  }
  p1 = graphion_arena_alloc(&arena, 8, 8);
  p2 = graphion_arena_alloc(&arena, 16, 16);

  if (p1 == NULL || p2 == NULL) {
    return 2;
  }
  if (((uintptr_t)p1 % 8U) != 0U || ((uintptr_t)p2 % 16U) != 0U) {
    return 3;
  }

  graphion_arena_reset(&arena);
  if (arena.offset != 0U) {
    return 4;
  }
  return 0;
}

int test_arena_invalid_alignment_fails(void) {
  uint8_t backing[64];
  graphion_arena arena;
  void *p;

  if (graphion_arena_init(&arena, backing, sizeof(backing)) != 0) {
    return 1;
  }
  p = graphion_arena_alloc(&arena, 8, 3);
  if (p != NULL) {
    return 2;
  }
  return 0;
}

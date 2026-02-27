#include "runtime/arena.h"

#include <stddef.h>
#include <stdint.h>

static size_t align_up(size_t value, size_t alignment) {
  const size_t mask = alignment - 1U;
  return (value + mask) & ~mask;
}

int graphion_arena_init(graphion_arena *arena, void *buffer, size_t capacity) {
  if (arena == NULL || buffer == NULL || capacity == 0U) {
    return -1;
  }
  arena->base = (uint8_t *)buffer;
  arena->capacity = capacity;
  arena->offset = 0U;
  return 0;
}

void *graphion_arena_alloc(graphion_arena *arena, size_t size, size_t alignment) {
  size_t at;
  size_t end;

  if (arena == NULL || size == 0U) {
    return NULL;
  }
  if (alignment == 0U || (alignment & (alignment - 1U)) != 0U) {
    return NULL;
  }

  at = align_up(arena->offset, alignment);
  if (at > arena->capacity) {
    return NULL;
  }

  end = at + size;
  if (end < at || end > arena->capacity) {
    return NULL;
  }

  arena->offset = end;
  return arena->base + at;
}

void graphion_arena_reset(graphion_arena *arena) {
  if (arena == NULL) {
    return;
  }
  arena->offset = 0U;
}

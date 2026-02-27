#ifndef GRAPHION_RUNTIME_ARENA_H
#define GRAPHION_RUNTIME_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *base;
  size_t capacity;
  size_t offset;
} graphion_arena;

int graphion_arena_init(graphion_arena *arena, void *buffer, size_t capacity);
void *graphion_arena_alloc(graphion_arena *arena, size_t size, size_t alignment);
void graphion_arena_reset(graphion_arena *arena);

#endif

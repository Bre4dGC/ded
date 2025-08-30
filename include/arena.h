// Copyright 2022 Alexey Kutepov <reximkut@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef ARENA_H_
#define ARENA_H_

#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_ASSERT
#include <assert.h>
#define ARENA_ASSERT assert
#endif

#define ARENA_BACKEND_LIBC_MALLOC 0
#define ARENA_BACKEND_LINUX_MMAP 1
#define ARENA_BACKEND_WIN32_VIRTUALALLOC 2
#define ARENA_BACKEND_WASM_HEAPBASE 3

#ifndef ARENA_BACKEND
#define ARENA_BACKEND ARENA_BACKEND_LIBC_MALLOC
#endif // ARENA_BACKEND

typedef struct Region Region;

struct Region
{
    Region *next;
    size_t count;
    size_t capacity;
    uintptr_t data[];
};

typedef struct
{
    Region *begin, *end;
} Arena;

#define REGION_DEFAULT_CAPACITY (8 * 1024)

Region *new_region(size_t capacity);
void free_region(Region *reg);

// TODO: snapshot/rewind capability for the arena
// - Snapshot should be combination of a->end and a->end->count.
// - Rewinding should be restoring a->end and a->end->count from the snapshot and
// setting count-s of all the Region-s after the remembered a->end to 0.
void *arena_alloc(Arena *arena, size_t size_bytes);
void *arena_realloc(Arena *arena, void *oldptr, size_t oldsz, size_t newsz);

void arena_reset(Arena *arena);
void arena_free(Arena *arena);

#endif // ARENA_H_

#ifdef ARENA_IMPLEMENTATION

#if ARENA_BACKEND == ARENA_BACKEND_LIBC_MALLOC
#include <stdlib.h>

// TODO: instead of accepting specific capacity new_region() should accept the size of the object we want to fit into the region
// It should be up to new_region() to decide the actual capacity to allocate
Region *new_region(size_t capacity)
{
    size_t size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
    // TODO: it would be nice if we could guarantee that the regions are allocated by ARENA_BACKEND_LIBC_MALLOC are page aligned
    Region *reg = malloc(size_bytes);
    ARENA_ASSERT(reg);
    reg->next = NULL;
    reg->count = 0;
    reg->capacity = capacity;
    return reg;
}

void free_region(Region *r)
{
    free(r);
}
#elif ARENA_BACKEND == ARENA_BACKEND_LINUX_MMAP
#error "TODO: Linux mmap backend is not implemented yet"
#elif ARENA_BACKEND == ARENA_BACKEND_WIN32_VIRTUALALLOC
#error "TODO: Win32 VirtualAlloc backend is not implemented yet"
#elif ARENA_BACKEND == ARENA_BACKEND_WASM_HEAPBASE
#error "TODO: WASM __heap_base backend is not implemented yet"
#else
#error "Unknown Arena backend"
#endif

// TODO: add debug statistic collection mode for arena
// Should collect things like:
// - How many times new_region was called
// - How many times existing region was skipped
// - How many times allocation exceeded REGION_DEFAULT_CAPACITY

void *arena_alloc(Arena *arena, size_t size_bytes)
{
    size_t size = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (arena->end == NULL)
    {
        ARENA_ASSERT(arena->begin == NULL);
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size)
            capacity = size;
        arena->end = new_region(capacity);
        arena->begin = arena->end;
    }

    while (arena->end->count + size > arena->end->capacity && arena->end->next != NULL)
    {
        arena->end = arena->end->next;
    }

    if (arena->end->count + size > arena->end->capacity)
    {
        ARENA_ASSERT(arena->end->next == NULL);
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size)
            capacity = size;
        arena->end->next = new_region(capacity);
        arena->end = arena->end->next;
    }

    void *result = &arena->end->data[arena->end->count];
    arena->end->count += size;
    return result;
}

void *arena_realloc(Arena *arena, void *oldptr, size_t oldsz, size_t newsz)
{
    if (newsz <= oldsz)
        return oldptr;
    void *newptr = arena_alloc(arena, newsz);
    char *newptr_char = newptr;
    char *oldptr_char = oldptr;
    for (size_t i = 0; i < oldsz; ++i)
    {
        newptr_char[i] = oldptr_char[i];
    }
    return newptr;
}

void arena_reset(Arena *arena)
{
    for (Region *r = arena->begin; r != NULL; r = r->next)
    {
        r->count = 0;
    }

    arena->end = arena->begin;
}

void arena_free(Arena *arena)
{
    Region *r = arena->begin;
    while (r)
    {
        Region *r0 = r;
        r = r->next;
        free_region(r0);
    }
    arena->begin = NULL;
    arena->end = NULL;
}

#endif // ARENA_IMPLEMENTATION

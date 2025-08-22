//
//  "arena.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "alloc/arena.h"

#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>
#else
#include <sys/mman.h>
#endif

// #define __USE_MISC

// NOTE: DEBUG LINE!! REMOVE!
// #define FPXLIBC_DEBUG

#define FPX_ARENA_META_SPACE (512 + 16)

struct __fpx_region {
    fpx_region* __next;
    fpx_region* __prev;
    void* __data;
    uint32_t __length;
    uint32_t __is_free;
};

struct __fpx_arena {
    fpx_region* __regions;
    uint32_t __region_count;
    uint32_t __size;
};

#ifdef FPXLIBC_DEBUG
static void arena_print(fpx_arena* arena) {
  printf("HEAD");
  for (fpx_region* reg = arena->__regions; reg; reg = reg->__next) {
    printf("->%p", reg);
  }
  printf("\n");
  fflush(stdout);
}
#endif

// #ifndef __FPXLIBC_ASM
fpx_arena* fpx_arena_create(uint16_t size) {

  size_t memsize =
    size + FPX_ARENA_META_SPACE;  // we also include room for the arena's metadata onto
                                  // this new allocation;
                                  // 512 bytes allows (512/32=)16 regions
  uint8_t* ptr = NULL;

#if defined(_WIN32) || defined(_WIN64)
  ptr = malloc(memsize);
  if (NULL == ptr)
    return (fpx_arena*)0;
#else
  if ((ptr = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) ==
    (void*)-1)
    return (fpx_arena*)0;
#endif

  fpx_region* reg = (fpx_region*)(ptr + 16);
  reg->__next = NULL;
  reg->__prev = NULL;
  reg->__data = ptr + 512 + 16;
  reg->__length = size;
  reg->__is_free = 0x1;

  fpx_arena* arena = (fpx_arena*)ptr;
  arena->__regions = reg;
  arena->__region_count = 1;
  arena->__size = size;

  return arena;
}
// #endif // __FPXLIBC_ASM

int fpx_arena_destroy(fpx_arena* ptr) {
  if (NULL == ptr)
    return -1;
#if defined(_WIN32) || defined(_WIN64)
  free(ptr);
  return 0;
#else
  return munmap(ptr, ptr->__size + FPX_ARENA_META_SPACE);
#endif
}

// if a nullptr is returned, it is because there is not enough space.
// this can be because of:
// - insufficient space
// - fragmentation
void* fpx_arena_alloc(fpx_arena* ptr, size_t size) {
  // old code ahead
  /*
  for (uint8_t i = 0; i < ptr->__region_count; ++i) {
    fpx_region* reg = &ptr->__regions[i];
    if (reg->__is_free && reg->__length >= size) {
      reg->__is_free = 0;
      if ((reg->__length == size) || (ptr->__region_count == 16)) {
        // return data pointer if size matches exactly or it's the last allowed
        // region
        return reg->__data;
      } else {
        // split region in two; one free, the other not
        fpx_region* new = &ptr->__regions[ptr->__region_count];
        new->__data = reg->__data + size;
        new->__length = reg->__length - size;
        new->__is_free = 1;
        reg->__length = size;
        ptr->__region_count++;
        return reg->__data;
      }
      break;
    }
  }
  */

  void* dataptr = NULL;
  fpx_region* reg = ptr->__regions;
  fpx_region* splitter = NULL;
  fpx_region* splittee = NULL;
  for (uint32_t i = 0; i < ptr->__region_count; ++i) {
    if (0 == reg->__length && !splittee) {
      splittee = reg;
      // have this new region be split into,
      // since it is of length 0 (and thus available)
      if (!splitter)
        continue;

      break;
    }

    if (reg->__is_free) {
      if (reg->__length == size || ptr->__region_count == 16) {
        // great! just take it
        dataptr = reg->__data;
        ptr->__region_count++;
        splitter = NULL;
        break;

      } else if (reg->__length < size) {
        splitter = NULL;

      } else if (!splitter) {
        splitter = reg;
        if (ptr->__region_count < 16)
          splittee = &ptr->__regions[ptr->__region_count];
        // split into "splittee" region if exists,
        // otherwise continue the loop and get splittee later
        if (!splittee)
          continue;

        break;
      }
    }

    reg = reg->__next;
  }

  if (splitter && splittee) {
    // splittee becomes the new region of which the data_ptr is returned
    // splitter remains with the rest of the length

    splitter->__length -= size;

    splittee->__length = size;
    splittee->__is_free = 0x0;
    splittee->__prev = splitter;
    splittee->__next = splitter->__next;
    splittee->__data = (uint8_t*)(splitter->__data) + splitter->__length;

    if (splitter->__next)
      splitter->__next->__prev = splittee;

    splitter->__next = splittee;

    dataptr = splittee->__data;

    ptr->__region_count++;
  }

#ifdef FPXLIBC_DEBUG
  arena_print(ptr);
#endif
  return dataptr;
}

int fpx_arena_free(fpx_arena* arenaptr, void* data) {
  fpx_region* regptr = arenaptr->__regions;

  while (regptr->__next && (regptr->__data != data))
    regptr = regptr->__next;

  if (regptr->__data != data)
    return 0;

  // if this region is the last in the __regions array, we just scrap it
  // and give its length to the closest other free region we find
  if (arenaptr->__regions[arenaptr->__region_count - 1].__data == data) {
    for (int i = arenaptr->__region_count - 1; i > 0; --i) {
      fpx_region* reg = &arenaptr->__regions[i - 1];
      if (reg->__is_free) {
        reg->__length += regptr->__length;
      }
    }

    if (regptr->__prev)
      regptr->__prev->__next = regptr->__next;

#ifdef FPXLIBC_DEBUG
    arena_print(arenaptr);
#endif

    return 1;
  }

  fpx_region* next = regptr->__next;
  while (next && next->__is_free) {
    regptr->__next = next->__next;
    regptr->__length += next->__length;
    next->__data = (uint8_t*)(next->__data) + next->__length;
    next->__length = 0;

    next = regptr->__next;
  }

  fpx_region* prev = regptr->__prev;
  while (prev && prev->__is_free) {
    regptr->__prev = prev->__prev;
    regptr->__length += prev->__length;
    regptr->__data = prev->__data;
    prev->__length = 0;

    prev = regptr->__prev;
  }

#ifdef FPXLIBC_DEBUG
  arena_print(arenaptr);
#endif

  return 1;
}

#ifdef FPX_RESET_DEBUG
#undef DEBUG
#endif

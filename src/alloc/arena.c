//
//  "arena.c"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "alloc/arena.h"
#include "fpx_debug.h"
#include "mem/mem.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>
#define PAGE_SIZE 4096
#else
#include <sys/mman.h>
#endif


struct __fpx_region {
    uint64_t __next_offset;  // UINT64_MAX means NONE
    uint64_t __prev_offset;  // UINT64_MAX means NONE
    void* __data;
    uint32_t __length;
    uint32_t __is_free;
};

struct __fpx_arena {
    fpx_region* __regions;
    uint32_t __region_count;
    uint32_t __region_capacity;
    uint32_t __size;
};

#define REG_NEXT(_arena, _region) \
  ((_region->__next_offset != UINT64_MAX) ? (_arena->__regions + _region->__next_offset) : NULL)
#define REG_PREV(_arena, _region) \
  ((_region->__prev_offset != UINT64_MAX) ? (_arena->__regions + _region->__prev_offset) : NULL)

static size_t page_size = 0;

static int _fpx_arena_double_reg_cap(fpx_arena* ptr);

#define FPX_ARENA_META_SPACE (sizeof(struct __fpx_arena))

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
fpx_arena* fpx_arena_create(uint64_t size) {

  uint64_t memsize = size + FPX_ARENA_META_SPACE;  // we also include room for the arena's metadata
                                                   // onto this new allocation;
  uint8_t* ar_ptr = NULL;
  uint8_t* reg_ptr = NULL;


#if defined(_WIN32) || defined(_WIN64)
  ar_ptr = malloc(memsize);
  if (NULL == ar_ptr)
    return (fpx_arena*)0;

  page_size = 4096;

  reg_ptr = malloc(page_size);
  if (NULL == reg_ptr) {
    free(ar_ptr);
    return (fpx_arena*)0;
  }
#else
  if ((ar_ptr = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) ==
    (void*)-1)
    return (fpx_arena*)0;

  page_size = getpagesize();

  if ((reg_ptr = mmap(0, page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) ==
    (void*)-1) {
    munmap(ar_ptr, memsize);
    return (fpx_arena*)0;
  }
#endif

  fpx_region* reg = (fpx_region*)reg_ptr;
  reg->__next_offset = reg->__prev_offset = UINT64_MAX;
  reg->__data = ar_ptr + FPX_ARENA_META_SPACE;
  reg->__length = size;
  reg->__is_free = 0x1;

  fpx_arena* arena = (fpx_arena*)ar_ptr;
  arena->__regions = reg;
  arena->__region_count = 1;
  arena->__region_capacity = page_size / sizeof(fpx_region);
  arena->__size = size;

  return arena;
}
// #endif // __FPXLIBC_ASM

int fpx_arena_destroy(fpx_arena* ptr) {
  if (NULL == ptr)
    return -1;
#if defined(_WIN32) || defined(_WIN64)
  free(ptr->__regions);
  free(ptr);
  return 0;
#else
  munmap(ptr->__regions, sizeof(fpx_region) * ptr->__region_capacity);
  munmap(ptr, ptr->__size + FPX_ARENA_META_SPACE);
#endif

  return 0;
}

// if a nullptr is returned, it is because there is not enough space.
// this can be because of:
// - insufficient space
// - fragmentation
void* fpx_arena_alloc(fpx_arena* ptr, size_t size) {
  if (NULL == ptr || 1 > size)
    return NULL;

  // old code ahead
  /*
  for (uint8_t i = 0; i < ptr->__region_count; ++i) {
    fpx_region* reg = &ptr->__regions[i];
    if (reg->__is_free && reg->__length >= size) {
      reg->__is_free = 0;
      if ((reg->__length == size) || (ptr->__region_count == ptr->__region_capacity)) {
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

  if (ptr->__region_count == ptr->__region_capacity)
    _fpx_arena_double_reg_cap(ptr);

  void* dataptr = NULL;
  fpx_region* reg = ptr->__regions;

  fpx_region* splitter = NULL;
  fpx_region* splittee = NULL;

  // fpx_region* last_reg = NULL;

  for (; reg != NULL; reg = REG_NEXT(ptr, reg)) {
    // // Debug:
    // if (reg->__is_free) {
    //   printf("free region- i: %u - size: %u\n", i, reg->__length);
    // }

    if (0 == reg->__length && !splittee) {
      splittee = reg;
      // have this new region be split into,
      // since it is of length 0 (and thus available)
      if (!splitter)
        continue;

      break;
    }

    if (reg->__is_free) {
      if (reg->__length == size) {
        // great! just take it
        dataptr = reg->__data;

        reg->__is_free = 0;

        ptr->__region_count++;
        splitter = NULL;
        break;

      } else if (reg->__length < size) {
        splitter = NULL;

      } else if (!splitter) {
        splitter = reg;
        if (ptr->__region_count < ptr->__region_capacity)
          splittee = &ptr->__regions[ptr->__region_count];
        // split into "splittee" region if exists,
        // otherwise continue the loop and get splittee later
        if (!splittee)
          continue;

        break;
      }
    }

    // last_reg = reg;

    // if ((uintptr_t)(reg + reg->__next_rela) > 140737488355327) {
    //   printf("%p\n", (void*)last_reg);
    // }
  }

  if (splitter && splittee) {
    // splittee becomes the new region of which the data_ptr is returned
    // splitter remains with the rest of the length

    splitter->__length -= size;

    splittee->__length = size;
    splittee->__is_free = 0x0;
    splittee->__prev_offset = splitter - ptr->__regions;

    if (UINT64_MAX == splitter->__next_offset) {
      splittee->__next_offset = UINT64_MAX;
    } else {
      splittee->__next_offset = REG_NEXT(ptr, splitter) - ptr->__regions;
    }

    splittee->__data = (uint8_t*)(splitter->__data) + splitter->__length;

    if (REG_NEXT(ptr, splitter)) {
      REG_NEXT(ptr, splitter)->__prev_offset = splittee - ptr->__regions;
    }

    splitter->__next_offset = splittee - ptr->__regions;
    dataptr = splittee->__data;

    ptr->__region_count++;
  }

#ifdef FPXLIBC_DEBUG
  arena_print(ptr);
#endif

  // static size_t total_alloced = 0;
  //
  // if (dataptr) {
  //   total_alloced += size;
  //   printf("%lu\n", total_alloced);
  // }

  return dataptr;
}

int fpx_arena_free(fpx_arena* arenaptr, void* data) {
  fpx_region* regptr = arenaptr->__regions;

  while (regptr->__next_offset != UINT64_MAX && (regptr->__data != data))
    REG_NEXT(arenaptr, regptr);

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

    if (NULL != REG_PREV(arenaptr, regptr)) {
      REG_PREV(arenaptr, regptr)->__next_offset = REG_NEXT(arenaptr, regptr) - arenaptr->__regions;
    }

#ifdef FPXLIBC_DEBUG
    arena_print(arenaptr);
#endif

    return 1;
  }

  // absorb :3
  fpx_region* next = REG_NEXT(arenaptr, regptr);
  while (next && next->__is_free) {
    regptr->__next_offset = next->__next_offset;
    regptr->__length += next->__length;
    next->__data = (uint8_t*)(next->__data) + next->__length;
    next->__length = 0;

    next = REG_NEXT(arenaptr, regptr);
  }

  // 3: brosba
  fpx_region* prev = REG_PREV(arenaptr, regptr);
  while (prev && prev->__is_free) {
    regptr->__prev_offset = prev->__prev_offset;
    regptr->__length += prev->__length;
    regptr->__data = prev->__data;
    prev->__length = 0;

    prev = REG_PREV(arenaptr, regptr);
  }

#ifdef FPXLIBC_DEBUG
  arena_print(arenaptr);
#endif

  return 1;
}

static int _fpx_arena_double_reg_cap(fpx_arena* ptr) {
  if (NULL == ptr)
    return -1;

  size_t new_capacity = ptr->__region_capacity * 2;
  fpx_region* new_ptr = NULL;

#if defined(_WIN32) || defined(_WIN64)
  new_ptr = malloc(new_capacity * sizeof(fpx_region));

  if (new_ptr == NULL)
    return -2;
#else
  new_ptr = mmap(0,
    new_capacity * sizeof(fpx_region),
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PRIVATE,
    -1,
    0);

  if (new_ptr == (void*)-1)
    return -2;
#endif


  fpx_memcpy(new_ptr, ptr->__regions, ptr->__region_count * sizeof(fpx_region));

#if defined(_WIN32) || defined(_WIN64)
  free(ptr->__regions);
#else
  if (munmap(ptr->__regions, ptr->__region_capacity * sizeof(fpx_region)) == -1) {
    munmap(new_ptr, new_capacity * sizeof(fpx_region));
    return -2;
  }
#endif

  ptr->__regions = new_ptr;
  ptr->__region_capacity = new_capacity;

  return 0;
}

#ifndef FPX_ARENA_H
#define FPX_ARENA_H

////////////////////////////////////////////////////////////////
//  "arena.h"                                                 //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

// x86_64 source code is in ./x86_64/arena.s

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

typedef struct __fpx_region fpx_region;
typedef struct __fpx_arena fpx_arena;

/**
 * Create a memory arena
 * Returns NULL/0 upon failure
 */
extern fpx_arena* fpx_arena_create(uint16_t size);

/**
 * Destroys an arena
 */
extern int fpx_arena_destroy(fpx_arena* ptr);

/**
 * Allocate memory within the arena pointed at by
 * the first argument.
 * The second argument (size) must be less than or
 * equal to the REMAINING space in this arena.
 */
extern void* fpx_arena_alloc(fpx_arena* ptr, size_t size);

extern int fpx_arena_free(fpx_arena* arenaptr, void* data);

#endif  // FPX_ARENA_H

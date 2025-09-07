extern "C" {
#include "alloc/arena.h"
}

#include "test/test-definitions.hpp"
#include <stdio.h>

int main() {
  fpx_arena *testptr_uwu = fpx_arena_create(1000);
  uint8_t *data = (uint8_t *)fpx_arena_alloc(testptr_uwu, 200);
  void *data2 = fpx_arena_alloc(testptr_uwu, 300);
  void *data3 = fpx_arena_alloc(testptr_uwu, 300);
  printf("(should be '(nil)' -> %p )\n\n", fpx_arena_alloc(testptr_uwu, 500));
  fpx_arena_free(testptr_uwu, data2);
  fpx_arena_free(testptr_uwu, data3);
  *data = 'a';
  *(data + 1) = 0;

  char testptr_value[16];
  char dataptr_value[16];

  snprintf(testptr_value, 15, "%p", (void *)testptr_uwu);
  snprintf(dataptr_value, 15, "%p", data);

  FPX_EXPECT(testptr_value, "0x7........000")
  EMPTY_LINE

  FPX_EXPECT(dataptr_value, "0x7........530")
  EMPTY_LINE

  FPX_EXPECT(data, "a")
  EMPTY_LINE

  return 0;
}

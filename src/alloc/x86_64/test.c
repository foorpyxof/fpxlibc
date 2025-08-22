#include "../arena.h"
#include <stdio.h>

int main() {
  
  fpx_arena* testptr_uwu = fpx_arena_create(1000);

  printf("0x%016x\n", testptr_uwu);

  return 0;

}

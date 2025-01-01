extern "C" {
  #include "fpx_mem/mem.h"
}

#include "test-definitions.h"

int main() {
  char* testptr = (char*)malloc(16);
  char arr[16];

  for(int i = 0; i < 15; ++i)
    arr[i] = 'a' + i;

  arr[15] = 0;
  // arr is now "abcdefghijklmno"

  fpx_memcpy(testptr, arr, sizeof(arr));
  FPX_EXPECT("abcdefghijklmno", testptr)

  EMPTY_LINE

  fpx_memset(testptr, 'w', 15);
  FPX_EXPECT("wwwwwwwwwwwwwww", testptr)

  return 0;
}

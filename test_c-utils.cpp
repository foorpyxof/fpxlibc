#include "test-definitions.h"

extern "C" {
	#include "fpx_c-utils/crypto.h"
	#include "fpx_c-utils/endian.h"
}

int main() {
  uint16_t testShort = 1;
	printf("%hu (expected: 1)\n", testShort);
  fpx_endian_swap(&testShort, 2);
  printf("%hu (expected: 256)\n", testShort);

  return 0;
}

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

  EMPTY_LINE

  printf("SHA1 test:\n");
  char output[40];
  fpx_sha1_digest("abcdefg", 7, output, TRUE);
  printf("           %s\n(expected: 2fb5e13419fc89246865e7a324f476ec624e8740)\n", output);
  
  EMPTY_LINE

  printf("base64 test:\n");
  char* output2 = fpx_base64_encode("abcdefg", 7);
  printf("           %s\n(expected: YWJjZGVmZw==)\n", output2);

  free(output2);

  return 0;
}

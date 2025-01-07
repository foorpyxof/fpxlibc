#include "test-definitions.h"
#include <cstdio>

extern "C" {
	#include "fpx_c-utils/crypto.h"
	#include "fpx_c-utils/endian.h"
  #include "fpx_c-utils/format.h"
}

int main() {
  uint16_t testShort = 1;
	printf("%hu (expected: 1)\n", testShort);
  fpx_endian_swap(&testShort, 2);
  printf("%hu (expected: 256)\n", testShort);

  EMPTY_LINE

  char output[40];
  fpx_sha1_digest("abcdefg", 7, output, TRUE);
  printf("SHA1 test:\n");
  FPX_EXPECT(output, "2fb5e13419fc89246865e7a324f476ec624e8740");

  EMPTY_LINE

  char* output2 = fpx_base64_encode("abcdefg", 7);
  printf("base64 test:\n");
  FPX_EXPECT(output2, "YWJjZGVmZw==")

  free(output2);

  EMPTY_LINE

  char numoutput[32] = { 0 };
  snprintf(numoutput, 31, "%d", fpx_strint((char*)"192837465"));
  printf("cstring-to-int test\n");
  FPX_EXPECT(numoutput, "192837465")
  snprintf(numoutput, 31, "%d", fpx_strint((char*)"-3067"));
  FPX_EXPECT(numoutput, "-3067")

  return 0;
}

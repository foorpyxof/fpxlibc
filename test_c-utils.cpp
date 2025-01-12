#include "test-definitions.h"

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

  printf("SHA1 test:\n");

  char output1[40];
  fpx_sha1_digest("abcdefg", 7, output1, TRUE);
  FPX_EXPECT(output1, "2fb5e13419fc89246865e7a324f476ec624e8740");

  EMPTY_LINE

  printf("base64 test:\n");

  char* output2 = fpx_base64_encode("abcdefg", 7);
  FPX_EXPECT(output2, "YWJjZGVmZw==")

  free(output2);

  EMPTY_LINE

  printf("cstring-to-int test\n");

  char output3[32] = { 0 };
  snprintf(output3, 31, "%d", fpx_strint((char*)"192837465"));
  FPX_EXPECT(output3, "192837465")
  snprintf(output3, 31, "%d", fpx_strint((char*)"-3067"));
  FPX_EXPECT(output3, "-3067")

  EMPTY_LINE

  printf("int-to-string test\n");

  char output4[32] = { 0 };
  fpx_intstr(12345, output4);
  FPX_EXPECT(output4, "12345")
  memset(output4, 0, sizeof(output4));
  fpx_intstr(192837465, output4);
  FPX_EXPECT(output4, "192837465")
  memset(output4, 0, sizeof(output4));
  fpx_intstr(0, output4);
  FPX_EXPECT(output4, "0")

  return 0;
}

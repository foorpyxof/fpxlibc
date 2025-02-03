#include "test-definitions.h"

extern "C" {
	#include "../fpx_c-utils/crypto.h"
	#include "../fpx_c-utils/endian.h"
  #include "../fpx_c-utils/format.h"
}

int main() {
  printf("Endian-swapper test:\n");

  uint16_t testShort = 1;
	char output1[16] = { 0 };
  fpx_endian_swap(&testShort, 2);
  snprintf(output1, 15, "%hu", testShort);
  FPX_EXPECT(output1, "256")

  EMPTY_LINE

  printf("SHA1 test:\n");

  char output2[40];
  fpx_sha1_digest("abcdefg", 7, output2, TRUE);
  FPX_EXPECT(output2, "2fb5e13419fc89246865e7a324f476ec624e8740")

  EMPTY_LINE

  printf("base64 test:\n");

  char* output3 = fpx_base64_encode("abcdefg", 7);
  FPX_EXPECT(output3, "YWJjZGVmZw==")

  free(output3);

  EMPTY_LINE

  printf("cstring-to-int test\n");

  char output4[32] = { 0 };
  snprintf(output4, 31, "%d", fpx_strint((char*)"192837465"));
  FPX_EXPECT(output4, "192837465")
  snprintf(output4, 31, "%d", fpx_strint((char*)"-3067"));
  FPX_EXPECT(output4, "-3067")

  EMPTY_LINE

  printf("int-to-string test\n");

  char output5[32] = { 0 };
  fpx_intstr(12345, output5);
  FPX_EXPECT(output5, "12345")
  memset(output5, 0, sizeof(output5));
  fpx_intstr(1928374650, output5);
  FPX_EXPECT(output5, "1928374650")
  memset(output5, 0, sizeof(output5));
  fpx_intstr(0, output5);
  FPX_EXPECT(output5, "0")
  memset(output5, 0, sizeof(output5));
  fpx_intstr(-1234, output5);
  FPX_EXPECT(output5, "-1234")

  EMPTY_LINE

  printf("hex-string test\n");

  char output6[32] = { 0 };
  uint32_t hextester1 = 3735928559;
  fpx_hexstr(&hextester1, sizeof(hextester1), output6, sizeof(output6));
  FPX_EXPECT(output6, "deadbeef")
  memset(output6, 0, sizeof(output6));
  uint16_t hextester2 = 300;
  fpx_hexstr(&hextester2, sizeof(hextester2), output6, sizeof(output6));
  FPX_EXPECT(output6, "12c")
  memset(output6, 0, sizeof(output6));
  uint16_t hextester3 = 273;
  fpx_hexstr(&hextester3, sizeof(hextester3), output6, sizeof(output6));
  FPX_EXPECT(output6, "111")
  memset(output6, 0, sizeof(output6));
  uint16_t hextester4 = 4095;
  fpx_hexstr(&hextester4, sizeof(hextester4), output6, sizeof(output6));
  FPX_EXPECT(output6, "fff")
  memset(output6, 0, sizeof(output6));
  uint8_t hextester5 = 0;
  fpx_hexstr(&hextester5, sizeof(hextester5), output6, sizeof(output6));
  FPX_EXPECT(output6, "0")

  return 0;
}

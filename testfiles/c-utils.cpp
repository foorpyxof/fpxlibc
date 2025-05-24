#include "test-definitions.h"

extern "C" {
#include "../fpx_c-utils/crypto.h"
#include "../fpx_c-utils/endian.h"
#include "../fpx_c-utils/format.h"
#include "../fpx_mem/mem.h"
#include "../fpx_networking/netutils.h"
}

int main() {
  {
    printf("Endian-swapper test:\n");

    uint16_t testShort = 1;
    char output1[16] = { 0 };
    fpx_endian_swap(&testShort, 2);
    snprintf(output1, 15, "%hu", testShort);
    FPX_EXPECT(output1, "256")
  }

  EMPTY_LINE

  {
    printf("base64 test:\n");

    char* output3 = fpx_base64_encode((uint8_t*)"abcdefg", 7);
    FPX_EXPECT(output3, "YWJjZGVmZw==")

    free(output3);
  }

  EMPTY_LINE

  {
    printf("cstring-to-int test:\n");

    char output4[32] = { 0 };
    snprintf(output4, 31, "%d", fpx_strint((char*)"192837465"));
    FPX_EXPECT(output4, "192837465")
    snprintf(output4, 31, "%d", fpx_strint((char*)"-3067"));
    FPX_EXPECT(output4, "-3067")
  }

  EMPTY_LINE

  {
    printf("int-to-string test:\n");

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
  }

  EMPTY_LINE

  {
    printf("hex-string test:\n");

    char output6[32] = { 0 };
    uint32_t hextester1 = 3735928559;
    fpx_hexstr(&hextester1, sizeof(hextester1), output6, sizeof(output6));
    FPX_EXPECT(output6, "deadbeef")
    memset(output6, 0, sizeof(output6));
    uint16_t hextester2 = 300;
    fpx_hexstr(&hextester2, sizeof(hextester2), output6, sizeof(output6));
    FPX_EXPECT(output6, "012c")
    memset(output6, 0, sizeof(output6));
    uint16_t hextester3 = 273;
    fpx_hexstr(&hextester3, sizeof(hextester3), output6, sizeof(output6));
    FPX_EXPECT(output6, "0111")
    memset(output6, 0, sizeof(output6));
    uint16_t hextester4 = 4095;
    fpx_hexstr(&hextester4, sizeof(hextester4), output6, sizeof(output6));
    FPX_EXPECT(output6, "0fff")
    memset(output6, 0, sizeof(output6));
    uint8_t hextester5 = 0;
    fpx_hexstr(&hextester5, sizeof(hextester5), output6, sizeof(output6));
    FPX_EXPECT(output6, "00")
  }

  EMPTY_LINE

  {
    printf("SHA1 test:\n");

    char output2[40];
    fpx_sha1_digest((uint8_t*)"abcdefg", 7, (uint8_t*)output2, TRUE);
    FPX_EXPECT(output2, "2fb5e13419fc89246865e7a324f476ec624e8740")
  }

  EMPTY_LINE

  {
    printf("HMAC-SHA1 test:\n");

    char output1[20];
    fpx_hmac((uint8_t*)"hahaha", 6, (uint8_t*)"abcdef", 6, (uint8_t*)output1, SHA1);
    char output2[41] = { 0 };
    for (int i = 0; i < 20; ++i)
      fpx_hexstr(&output1[i], 1, &output2[i * 2], 2);

    FPX_EXPECT(output2, "15b6007b8bf9d9032f41ef43f75cfc74527eb91f")

    char output3[20];
    fpx_hmac((uint8_t*)"you will never guess this key!!!",
      32,
      (uint8_t*)"asdhHJKSDHWHSDAJHiasukdhGKWSiudhUAGDHAS",
      39,
      (uint8_t*)output3,
      SHA1);
    char output4[41] = { 0 };
    for (int i = 0; i < 20; ++i)
      fpx_hexstr(&output3[i], 1, &output4[i * 2], 2);

    FPX_EXPECT(output4, "e2ac72ca2845edad5c1f77f5462dc808bb608dd1")

    char output5[20];
    fpx_hmac((uint8_t*)"b", 1, (uint8_t*)"a", 1, (uint8_t*)output5, SHA1);
    char output6[41] = { 0 };
    for (int i = 0; i < 20; ++i)
      fpx_hexstr(&output5[i], 1, &output6[i * 2], 2);

    FPX_EXPECT(output6, "8abe0fd691e3da3035f7b7ac91be45d99e942b9e")
  }

  EMPTY_LINE

  {
    printf("SHA-256 test:\n");

    uint8_t input1[] = "abcdefghijkl";
    uint8_t output1[65] = { 0 };
    fpx_sha256_digest(input1, sizeof(input1) - 1, output1, 1);
    FPX_EXPECT(output1, "d682ed4ca4d989c134ec94f1551e1ec580dd6d5a6ecde9f3d35e6e4a717fbde4")

    uint8_t input2[] =
      "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzghijklmnopqrstuvwxyzab";
    uint8_t output2[65] = { 0 };
    fpx_sha256_digest(input2, sizeof(input2) - 1, output2, 1);
    FPX_EXPECT(output2, "c99255c79746b5d3e9d58d2a6c9d6c91e620dfe152f301b38caf5e1ec43e377e")

    uint8_t input3[] =
      "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzghijklmnopqrstuvwxyz";
    uint8_t output3[65] = { 0 };
    fpx_sha256_digest(input3, sizeof(input3) - 1, output3, 1);
    FPX_EXPECT(output3, "5eafc33ce66722eb020d6fe703b0b3e97afbc7aa9a011fd9e8bafe684f97d541")
  }

  EMPTY_LINE

  {
    printf("HMAC-SHA256 test:\n");

    char output1[32];
    fpx_hmac((uint8_t*)"hahaha", 6, (uint8_t*)"abcdef", 6, (uint8_t*)output1, SHA256);
    char output2[65] = { 0 };
    for (int i = 0; i < 32; ++i)
      fpx_hexstr(&output1[i], 1, &output2[i * 2], 2);

    FPX_EXPECT(output2, "2c9da5de109f26008ab955d869a2c1adf95b12ddf983288ea75dca52b3221317")
  }

  EMPTY_LINE

  {
    printf("HKDF-SHA1 test:\n");

    uint8_t ikm1[6];
    fpx_memcpy(ikm1, "abcdef", sizeof(ikm1));

    uint8_t salt1[6];
    fpx_memcpy(salt1, "abcdef", sizeof(salt1));

    uint8_t info1[6];
    fpx_memcpy(info1, "abcdef", sizeof(info1));

    uint8_t output1[32] = { 0 };

    fpx_hkdf_extract(salt1, sizeof(salt1), ikm1, sizeof(ikm1), output1, SHA1);
    fpx_hkdf_expand(output1, info1, sizeof(info1), output1, sizeof(output1), SHA1);

    char outbuf1[2 * sizeof(output1) + 1] = { 0 };

    for (int i = 0; i < sizeof(output1); ++i) {
      fpx_hexstr(&output1[i], 1, outbuf1 + i * 2, 2);
    }

    FPX_EXPECT(outbuf1, "b5be57164b23d5ff63325e3fe8cd342df2807b2dbe99989605e8360d2b4a8ed4")

    uint8_t ikm2[12];
    fpx_memcpy(ikm2, "jeosydhrvelo", sizeof(ikm2));

    uint8_t salt2[20];
    fpx_memcpy(salt2, "aleuqfcblwpftrzihgq0", sizeof(salt2));

    uint8_t info2[16];
    fpx_memcpy(info2, "p15ch4975fhalth3", sizeof(info2));

    uint8_t output2[20] = { 0 };

    fpx_hkdf_extract(salt2, sizeof(salt2), ikm2, sizeof(ikm2), output2, SHA1);
    fpx_hkdf_expand(output2, info2, sizeof(info2), output2, sizeof(output2), SHA1);

    char outbuf2[2 * sizeof(output2) + 1] = { 0 };

    for (int i = 0; i < sizeof(output2); ++i) {
      fpx_hexstr(&output2[i], 1, outbuf2 + i * 2, 2);
    }

    FPX_EXPECT(outbuf2, "ef95da2339e8752931fde7fe697042e823fc942c")
  }

  EMPTY_LINE

  {
    printf("HKDF-SHA256 test:\n");

    uint8_t ikm1[6];
    fpx_memcpy(ikm1, "abcdef", sizeof(ikm1));

    uint8_t salt1[6];
    fpx_memcpy(salt1, "abcdef", sizeof(salt1));

    uint8_t info1[6];
    fpx_memcpy(info1, "abcdef", sizeof(info1));

    uint8_t output1[32] = { 0 };

    fpx_hkdf_extract(salt1, sizeof(salt1), ikm1, sizeof(ikm1), output1, SHA256);
    fpx_hkdf_expand(output1, info1, sizeof(info1), output1, sizeof(output1), SHA256);

    char outbuf1[2 * sizeof(output1) + 1] = { 0 };

    for (int i = 0; i < sizeof(output1); ++i) {
      fpx_hexstr(&output1[i], 1, outbuf1 + i * 2, 2);
    }

    FPX_EXPECT(outbuf1, "29a2feab48326d21a9e20563dc9ef580d2d4bd7932f3c0b20e6caee2ea867e79")

    uint8_t ikm2[12];
    fpx_memcpy(ikm2, "jeosydhrvelo", sizeof(ikm2));

    uint8_t salt2[20];
    fpx_memcpy(salt2, "aleuqfcblwpftrzihgq0", sizeof(salt2));

    uint8_t info2[16];
    fpx_memcpy(info2, "p15ch4975fhalth3", sizeof(info2));

    uint8_t output2[48] = { 0 };

    fpx_hkdf_extract(salt2, sizeof(salt2), ikm2, sizeof(ikm2), output2, SHA256);
    fpx_hkdf_expand(output2, info2, sizeof(info2), output2, sizeof(output2), SHA256);

    char outbuf2[2 * sizeof(output2) + 1] = { 0 };

    for (int i = 0; i < sizeof(output2); ++i) {
      fpx_hexstr(&output2[i], 1, outbuf2 + i * 2, 2);
    }

    FPX_EXPECT(outbuf2,
      "48142b8f2e6569b8895b8fba2220c5c9a8a667f7d978d35bab8b6a90d39cf859809759b17d052450f0c98626c5a7aefd")
  }

  return 0;
}

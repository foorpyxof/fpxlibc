#include "test-definitions.h"

extern "C" {
  #include "fpx_math/math.h"
}

int main() {

  char output1[32] = { 0 };
  snprintf(output1, 31, "%d", fpx_pow(10, 3));
  FPX_EXPECT(output1, "1000")
  memset(output1, 0, sizeof(output1));
  snprintf(output1, 31, "%d", fpx_pow(5, 5));
  FPX_EXPECT(output1, "3125")
  memset(output1, 0, sizeof(output1));
  snprintf(output1, 31, "%d", fpx_pow(2, 0));
  FPX_EXPECT(output1, "1")

  return 0;
}

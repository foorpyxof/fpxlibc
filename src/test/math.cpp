#include "test/test-definitions.hpp"

extern "C" {
#include "math/math.h"
}

int main() {

  printf("Testing power-raising\n");

  char output1[32] = { 0 };
  snprintf(output1, 31, "%d", fpx_pow(10, 3));
  printf("\n 10 to the power of 3\n");
  FPX_EXPECT(output1, "1000")

  memset(output1, 0, sizeof(output1));
  snprintf(output1, 31, "%d", fpx_pow(5, 5));
  printf("\n 5 to the power of 5\n");
  FPX_EXPECT(output1, "3125")

  memset(output1, 0, sizeof(output1));
  snprintf(output1, 31, "%d", fpx_pow(2, 0));
  printf("\n 2 to the power of 0\n");
  FPX_EXPECT(output1, "1")

  return 0;
}

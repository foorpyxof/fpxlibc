#include "test/test-definitions.hpp"
extern "C" {
#include "string/string.h"
}

int main() {

  const char* testString = "hELLo friENd";
  char* lowercaseTestString = fpx_string_to_lower(testString, true);

  std::cout << "String: " << testString << std::endl;  // expected output: hELLo friENd
  std::cout << "String length: " << fpx_getstringlength(testString)
            << std::endl;  // expected output: 12
  std::cout << "String in lowercase: " << lowercaseTestString
            << std::endl;  // expected output: hello friend

  free(lowercaseTestString);

  char array[16] = { 'a' };
  fpx_strcpy(array, (char*)"lalalala");

  FPX_EXPECT(array, "lalalala")
}

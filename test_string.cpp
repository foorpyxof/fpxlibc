#include "test-definitions.h"
extern "C" {
	#include "fpx_string/string.h"
}

int main() {

	const char* testString = "hELLo friENd";
  char* lowercaseTestString = fpx_string_to_lower(testString, true);

  std::cout << "String: " << testString << std::endl; // expected output: hELLo friENd
  std::cout << "String length: " << fpx_getstringlength(testString) << std::endl; // expected output: 12
  std::cout << "String in lowercase: " << lowercaseTestString << std::endl; // expected output: hello friend

  free(lowercaseTestString);

}
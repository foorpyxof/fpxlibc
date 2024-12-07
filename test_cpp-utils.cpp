#include "test-definitions.h"
#include "fpx_cpp-utils/exceptions.h"

using namespace fpx;

int main() {

	std::cout << "Starting exception tests:\n" << std::endl;

  // test fpx::Exception
  try { throw Exception(); }
  catch (Exception& err) { err.Print(); } // Error -1: An exception has occured at runtime!
	
  try { throw NotImplementedException("hiyaaa"); }
  catch (Exception& err) { err.Print(); } // Error -2: hiyaaa

  try { throw IndexOutOfRangeException(-69); }
  catch (Exception& err) { err.Print(); } // Error -69: The index you tried to reach is not in range!

  try { throw NetException("NetException test", -1100); }
  catch (Exception& err) { err.Print(); } // Error -1100: NetException test

  EMPTY_LINE

}
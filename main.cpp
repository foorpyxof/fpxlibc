#include "fpx_cpp-utils/fpx_cpp-utils.h"
#include "fpx_linkedlist/fpx_linkedlist.h"
extern "C" {
  #include "fpx_string/fpx_string.h"
}

using namespace fpx;

int main(int argc, const char** argv) {
  try { throw new Exception(); }
  catch (Exception* err) {std::cout << err->Message() << std::endl; } //An exception has occured at runtime!
  try { throw new NotImplementedException(); }
  catch (Exception* err) {std::cout << err->Message() << std::endl; } //This functionality has not been implemented yet!
  try { throw new IndexOutOfRangeException(); }
  catch (Exception* err) {std::cout << err->Message() << std::endl; } //The index you tried to reach is not in range!

  LinkedList ll;

  LinkedListNode* node1PTR = new LinkedListNode(123);

  ll.PrependNode(new LinkedListNode(4));
  ll.PrependNode(new LinkedListNode(9));

  std::cout << ll[0].Value << ' ' << ll[1].Value << std::endl; // expected output: 9 4

  ll.InsertNode(new LinkedListNode(21), 1);
  ll.InsertNode(node1PTR, 2);

  ll.Print(); //expected output: 9->21->123->4

  ll.RemoveNode(1);

  ll.Print(); //expected output: 9->123->4

  const char* testString = "hey";
  std::cout << "String length: " << fpx_getstringlength(testString) << std::endl; //expected output: 3

  return 0;
}
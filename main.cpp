#define EMPTY_LINE std::cout << std::endl;

#include "fpx_cpp-utils/fpx_cpp-utils.h"
#include "fpx_linkedlist/fpx_linkedlist.h"
extern "C" {
  #include "fpx_string/fpx_string.h"
}

using namespace fpx;

int main(int argc, const char** argv) {
  try { throw new Exception(); }
  catch (Exception* err) { err->Print(); } // An exception has occured at runtime!
  try { throw new NotImplementedException(); }
  catch (Exception* err) { err->Print(); } // This functionality has not been implemented yet!
  try { throw new IndexOutOfRangeException(); }
  catch (Exception* err) { err->Print(); } // The index you tried to reach is not in range!

  EMPTY_LINE

  LinkedList sll; // a singly linked list
  LinkedListNode* node1PTR = new LinkedListNode(123);

  sll.PrependNode(new LinkedListNode(4)); // add 4 to the start of the linked-list
  sll.PrependNode(new LinkedListNode(9)); // add 9 to the start of the linked-list

  std::cout << sll[0].Value << ' ' << sll[1].Value << std::endl; //  expected output: 9 4

  EMPTY_LINE

  sll.InsertNode(new LinkedListNode(21), 1); // insert new node with value 21 between index 0 and 1
  sll.InsertNode(node1PTR, 2); // insert pre-made node (123) between index 1 and 2

  sll.Print(); // expected output: 9->21->123->4

  EMPTY_LINE

  sll.RemoveNode(1); // remove node at index 1

  sll.SetName("My epic LinkedList");

  sll.Print(); // expected output: 9->123->4

  EMPTY_LINE

  const char* testString = "hELLo friENd";
  const char* testLowercase = fpx_string_to_lower(testString);

  std::cout << "String: " << testString << std::endl; // expected output: hELLo friENd
  std::cout << "String length: " << fpx_getstringlength(testString) << std::endl; // expected output: 12
  std::cout << "String in lowercase: " << testLowercase << std::endl; // expected output: hello friend

  delete[] testLowercase;

  return 0;
}
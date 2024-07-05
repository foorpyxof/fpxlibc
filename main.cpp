#define EMPTY_LINE std::cout << std::endl;

#include "fpx_cpp-utils/fpx_cpp-utils.h"
#include "fpx_linkedlist/fpx_linkedlist.h"
#include "fpx_vector/fpx_vector.h"
#include "fpx_networking/fpx_server/fpx_server.h"
extern "C" {
  #include "fpx_string/fpx_string.h"
}

using namespace fpx;

int main(int argc, const char** argv) {

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

  // test fpx::LinkedList
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

  // test fpx_string
  const char* testString = "hELLo friENd";
  const char* testLowercase = fpx_string_to_lower(testString);

  std::cout << "String: " << testString << std::endl; // expected output: hELLo friENd
  std::cout << "String length: " << fpx_getstringlength(testString) << std::endl; // expected output: 12
  std::cout << "String in lowercase: " << testLowercase << std::endl; // expected output: hello friend

  delete[] testLowercase;

  EMPTY_LINE

  // test fpx::Vector<>
  Vector<int> v1(4); // create a vector with capacity 4
  v1.PushBack(2); // add given value to the end of the vector
  v1.PushBack(-13);
  v1.PushBack(123);

  v1.Grow(3); // expand capacity by 3

  std::cout << "v1 size: " << v1.GetSize() << std::endl; // expect: 3
  std::cout << "v1 capacity: " << v1.GetCapacity() << std::endl; // expect: 7 (4+3)
  
  EMPTY_LINE
  
  for (int object : v1) {
    std::cout << object << std::endl;
  } // expect: 2 | -13 | 123
  
  EMPTY_LINE
  
  v1.PopBack();

  for (int object : v1) {
    std::cout << object << std::endl;
  } // expect: 2 | -13

  EMPTY_LINE

  Vector<char> v2; // create an empty vector
  std::cout << "v2 size: " << v2.GetSize() << std::endl; // expect: 0
  std::cout << "v2 capacity: " << v2.GetCapacity() << std::endl; // expect: 0
  
  EMPTY_LINE
  
  v2.PushBack('h');
  v2.PushBack('i');

  std::cout << "v2 size: " << v2.GetSize() << std::endl; // expect: 2
  std::cout << "v2 capacity: " << v2.GetCapacity() << std::endl; // expect: 4
  for (char object : v2) {
    std::cout << object << std::endl;
  } // expect h | i
  
  EMPTY_LINE
  
  v2.PopBack();
  std::cout << "v2 shrink by 4 succeeded? " << v2.Shrink(4) << std::endl; // expect: false/0
  std::cout << "v2 capacity: " << v2.GetCapacity() << std::endl; // expect: 4
  
  EMPTY_LINE
  
  std::cout << "v2 empty? " << v2.IsEmpty() << std::endl; // expect: false/0
  v2.PopBack();
  std::cout << "v2 empty? " << v2.IsEmpty() << std::endl; // expect: true/1

  v2.PushBack('u');
  v2.PushBack('w');
  v2.PushBack('u');

  Vector<char> v3;
  v3.PushBack('h');
  v3.PushBack('i');
  v3.PushBack(' ');
  v3.PushBack('t');
  v3.PushBack('h');
  v3.PushBack('e');
  v3.PushBack('r');
  v3.PushBack('e');
  v3.PushBack('!');
  v3.PushBack('!');

  v2.PushBack(v3);

  for (char& obj : v3) std::cout << obj; // expect: v3 appended to v2
  std::cout << std::endl;

  EMPTY_LINE
  
  TcpServer::Setup("127.0.0.1", 9999);
  try {
    TcpServer::Listen();
  } catch (fpx::NetException exc) {
    exc.Print();
  }

  return 0;

}
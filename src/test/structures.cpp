#include <stdio.h>

#include "../fpx_structures/linkedlist.h"
#include "../fpx_structures/vector.h"
#include "test-definitions.h"

using namespace fpx;

int main() {

  // test fpx::LinkedList
  LinkedList sll;  // a singly linked list
  LinkedListNode* node1PTR = new LinkedListNode(123);

  sll.PrependNode(new LinkedListNode(4));  // add 4 to the start of the linked-list
  sll.PrependNode(new LinkedListNode(9));  // add 9 to the start of the linked-list

  char sllout1[8] = { 0 };
  snprintf(sllout1, sizeof(sllout1) - 1, "%d %d", sll[0].Value, sll[1].Value);
  FPX_EXPECT(sllout1, "9 4")

  EMPTY_LINE

  sll.InsertNode(new LinkedListNode(21), 1);  // insert new node with value 21 between index 0 and 1
  sll.InsertNode(node1PTR, 2);                // insert pre-made node (123) between index 1 and 2

  sll.Print();  // expected output: 9->21->123->4

  EMPTY_LINE

  sll.RemoveNode(1);  // remove node at index 1

  sll.SetName("My epic LinkedList");

  sll.Print();  // expected output: 9->123->4

  EMPTY_LINE

  // test fpx::Vector<>
  Vector<int> v1(4);  // create a vector with capacity 4
  v1.PushBack(2);     // add given value to the end of the vector
  v1.PushBack(-13);
  v1.PushBack(123);

  v1.Grow(3);  // expand capacity by 3

  char v1out1[8] = { 0 };

  std::cout << "v1 size:" << std::endl;
  snprintf(v1out1, sizeof(v1out1) - 1, "%u", v1.GetSize());
  FPX_EXPECT(v1out1, "3")
  memset(v1out1, 0, sizeof(v1out1));
  std::cout << "v1 capacity: " << std::endl;
  snprintf(v1out1, sizeof(v1out1) - 1, "%u", v1.GetCapacity());
  FPX_EXPECT(v1out1, "7")
  memset(v1out1, 0, sizeof(v1out1));

  EMPTY_LINE

  char v1out2[32] = { 0 };
  char* v1out2ptr = &v1out2[0];
  for (int object : v1) {
    v1out2ptr += snprintf(v1out2ptr, sizeof(v1out2) - 1, "%d ", object);
  }  // expect: 2 | -13 | 123
  FPX_EXPECT(v1out2, "2 -13 123")
  memset(v1out2, 0, sizeof(v1out2));

  EMPTY_LINE

  v1.PopBack();

  v1out2ptr = &v1out2[0];
  for (int object : v1) {
    v1out2ptr += snprintf(v1out2ptr, sizeof(v1out2) - 1, "%d ", object);
  }  // expect: 2 | -13
  FPX_EXPECT(v1out2, "2 -13");
  memset(v1out2, 0, sizeof(v1out2));

  EMPTY_LINE

  Vector<char> v2;                                                // create an empty vector

  char v2out1[8] = { 0 };

  std::cout << "v2 size:" << std::endl;
  snprintf(v2out1, sizeof(v2out1) - 1, "%u", v2.GetSize());
  FPX_EXPECT(v2out1, "0")
  memset(v2out1, 0, sizeof(v2out1));
  std::cout << "v2 capacity: " << std::endl;
  snprintf(v2out1, sizeof(v2out1) - 1, "%u", v2.GetCapacity());
  FPX_EXPECT(v2out1, "0")
  memset(v2out1, 0, sizeof(v2out1));

  EMPTY_LINE

  v2.PushBack('h');
  v2.PushBack('i');

  char v2out2[8] = { 0 };

  std::cout << "v2 size:" << std::endl;
  snprintf(v2out2, sizeof(v2out2) - 1, "%u", v2.GetSize());
  FPX_EXPECT(v2out2, "2")
  memset(v2out2, 0, sizeof(v2out2));
  std::cout << "v2 capacity: " << std::endl;
  snprintf(v2out2, sizeof(v2out2) - 1, "%u", v2.GetCapacity());
  FPX_EXPECT(v2out2, "4")
  memset(v2out2, 0, sizeof(v2out2));

  EMPTY_LINE

  char v2out3[32] = { 0 };
  char* v2out3ptr = &v2out3[0];
  for (char object : v2) {
    v2out3ptr += snprintf(v2out3ptr, sizeof(v2out3) - 1, "%c", object);
  }
  FPX_EXPECT(v2out3, "hi")  // expect h | i

  EMPTY_LINE

  char v2out4[8] = { 0 };

  v2.PopBack();
  std::cout << "v2 shrink by 4 succeeded? (expect false/0) " << v2.Shrink(4) << std::endl;  // expect: false/0

  EMPTY_LINE

  snprintf(v2out4, sizeof(v2out4) - 1, "%d", v2.GetCapacity());
  FPX_EXPECT(v2out4, "4")

  EMPTY_LINE

  std::cout << "v2 empty? (expect false/0) " << v2.IsEmpty() << std::endl;  // expect: false/0
  v2.PopBack();
  std::cout << "v2 empty? (expect true/1) " << v2.IsEmpty() << std::endl;  // expect: true/1

  EMPTY_LINE

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

  char v2out5[32] = { 0 };
  char* v2out5ptr = &v2out5[0];
  for (char& obj : v2)
    v2out5ptr += snprintf(v2out5ptr, sizeof(v2out5) - 1, "%c", obj);

  FPX_EXPECT(v2out5, "uwuhi there!!")

  EMPTY_LINE
}

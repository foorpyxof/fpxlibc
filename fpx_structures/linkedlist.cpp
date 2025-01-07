////////////////////////////////////////////////////////////////
//  "linkedlist.cpp"                                          //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "linkedlist.h"

namespace fpx {

LinkedList::~LinkedList() {
  // std::cout << "Linked List out of scope! Freeing nodes..." << std::endl;

  LinkedListNode* current = nullptr;

  while (m_FirstNode) {
    current = m_FirstNode;
    m_FirstNode = current->Next;
    delete current;
  }
}

unsigned int LinkedList::GetLength() const {
  return m_Length;
}


void LinkedList::PrependNode(LinkedListNode* node_Address) {
  node_Address->Next = m_FirstNode;
  m_FirstNode = node_Address;

  m_Length++;
  return;
}

void LinkedList::InsertNode(LinkedListNode* node_Address, unsigned int index = 0) {
  if (!index)
    PrependNode(node_Address);
  else
    node_Address->Next = &(*this)[index];
    (*this)[index-1].Next = node_Address;
    m_Length++;

  return;
}

void* LinkedList::RemoveNode(LinkedListNode* node_Address) {
  const LinkedListNode& node = *node_Address;

  for(LinkedListNode& nodeReference : *this) {
    if (nodeReference.Next == &node) {
      nodeReference.Next = node.Next;
      delete &node;
    }
  }

  m_Length--;
  return nullptr;
}

void* LinkedList::RemoveNode(unsigned int node_Index) {
  const LinkedListNode& node = (*this)[node_Index]; 
  switch(node_Index) {
    case 0:
      m_FirstNode = node.Next;
      delete &node;
    default:
      (*this)[node_Index-1].Next = node.Next;
      delete &node;
  }
  m_Length--;
  return nullptr;
}

void LinkedList::Print() const {
  std::cout << m_Name << ":" << std::endl << "(head)->";
  for(LinkedListNode& node : *this) {
    node.Print();
    if (node.Next) std::cout << "->";
  }
  std::cout << std::endl;
}


void LinkedList::SetName(const char* newName) { m_Name = newName; }
const char* LinkedList::GetName() const { return m_Name; }


LinkedListNode& LinkedList::operator[] (unsigned int index) const {
  LinkedListNode* current = m_FirstNode;

  if (nullptr == current) throw new IndexOutOfRangeException("This linked list is empty"); // list is empty

  if(0 == index) return *current; // immediately return first object

  for(int i=0; i<index; i++) {
    current = current->Next;
    if (nullptr == current) throw new IndexOutOfRangeException; // index out of range
  }

  return *current;
}

}

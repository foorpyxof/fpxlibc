#ifndef FPX_LINKEDLIST_H
#define FPX_LINKEDLIST_H

#include <iostream>
#include "../fpx_cpp-utils/fpx_cpp-utils.h"

namespace fpx {

struct LinkedListNode {
  LinkedListNode(): Next(nullptr), Value(0) {}
  LinkedListNode(int val): Next(nullptr), Value(val) {}

  // ~LinkedListNode() { std::cout << "Destructing node with value " << Value << std::endl; }

  virtual void Print() { std::cout << Value; }

  virtual bool operator==(LinkedListNode const& other) const { return Value == other.Value; }

  LinkedListNode* Next;
  int Value;
};

class LinkedList {
  public:
    LinkedList(): m_Name("unnamed list"), m_Length(0), m_FirstNode(nullptr) {}
    LinkedList(const char* name): m_Name(name), m_Length(0), m_FirstNode(nullptr) {}
    ~LinkedList();

    unsigned int GetLength() const;

    void PrependNode(LinkedListNode*);
    void InsertNode(LinkedListNode*, unsigned int);

    void* RemoveNode(LinkedListNode*);
    void* RemoveNode(unsigned int);

    void Print() const;

    void SetName(const char*);
    const char* GetName() const;

    LinkedListNode& operator[] (unsigned int) const;

    class Iterator {
      public:
        LinkedListNode* current;

        Iterator(LinkedListNode* node_Address): current(node_Address) {}

        LinkedListNode& operator*() { return *current; }
        Iterator& operator++() {
          if (current)
            current = current->Next;

          return *this;
        }
        Iterator operator++(int) {
          Iterator self = *this;
          (*this)++;

          return self;
        }

        bool operator==(Iterator const& other) const { return (current == other.current); }
        bool operator!=(Iterator const& other) const { return (current != other.current); }
    };

    Iterator begin() const { return Iterator(m_FirstNode); }
    Iterator end() const { return Iterator(nullptr); }

  private:
    const char* m_Name;
    unsigned int m_Length;
    LinkedListNode* m_FirstNode = nullptr;
};

}

#endif // FPX_LINKEDLIST_H
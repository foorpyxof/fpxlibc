#ifndef FPX_LINKEDLIST_H
#define FPX_LINKEDLIST_H

//
//  "linkedlist.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include <iostream>

namespace fpx {

struct LinkedListNode {
    /**
     * Create a new node. Default value is 0.
     */
    LinkedListNode(int val = 0) : Next(nullptr), Value(val) { }

    // ~LinkedListNode() { std::cout << "Destructing node with value " << Value << std::endl; }

    /**
     * Prints the value of the node to stdout.
     */
    virtual void Print() {
      std::cout << Value;
    }

    virtual bool operator==(LinkedListNode const& other) const {
      return Value == other.Value;
    }

    LinkedListNode* Next;
    int Value;
};

class LinkedList {
  public:
    /**
     * Create a new empty linked list
     */
    LinkedList(const char* name = "unnamed list") :
      m_Name(name),
      m_Length(0),
      m_FirstNode(nullptr) { }
    ~LinkedList();

    /**
     * Returns the length of the linked list.
     */
    unsigned int GetLength() const;

    /**
     * Adds a node in front of the list.
     */
    void PrependNode(LinkedListNode*);
    /**
     * Insert a node at the specified index, pushing any other nodes ahead if necessary.
     */
    void InsertNode(LinkedListNode*, unsigned int);

    /**
     * Remove a node from the linked list by node-pointer.
     */
    void* RemoveNode(LinkedListNode*);
    /**
     * Remove a node from the linked list by index.
     */
    void* RemoveNode(unsigned int);

    /**
     * Prints all linked list data, such as the name and the nodes and their values.
     */
    void Print() const;

    /**
     * Sets a name for the linked list.
     */
    void SetName(const char*);
    /**
     * Returns the name of the linked list.
     */
    const char* GetName() const;

    LinkedListNode& operator[](unsigned int) const;

    class Iterator {
      public:
        LinkedListNode* current;

        Iterator(LinkedListNode* node_Address) : current(node_Address) { }

        LinkedListNode& operator*() {
          return *current;
        }
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

        bool operator==(Iterator const& other) const {
          return (current == other.current);
        }
        bool operator!=(Iterator const& other) const {
          return (current != other.current);
        }
    };

    Iterator begin() const {
      return Iterator(m_FirstNode);
    }
    Iterator end() const {
      return Iterator(nullptr);
    }

  private:
    const char* m_Name;
    unsigned int m_Length;
    LinkedListNode* m_FirstNode = nullptr;
};

}  // namespace fpx

#endif  // FPX_LINKEDLIST_H

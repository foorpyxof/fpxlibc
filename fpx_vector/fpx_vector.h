#ifndef FPX_VECTOR_H
#define FPX_VECTOR_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include <iostream>
#include "../fpx_cpp-utils/fpx_cpp-utils.h"
#include <cmath>

namespace fpx {

template<typename T>
class Vector {
  public:
    Vector();
    Vector(unsigned int);
    Vector(T[]);
    ~Vector();

    unsigned int GetSize() const { return m_Size; }
    unsigned int GetCapacity() const { return m_Capacity; }
    static unsigned int MaxSize() { return m_MaxSize; }

    bool IsEmpty() const { return (m_Size == 0); }

    bool DoubleCapacity();
    bool Grow(const int& = 1);
    bool Shrink(const int& = 1);

    T& Front() const { return m_Array[0]; }
    T& Back() const { return m_Array[m_Size-1]; }

    T* Data() const { return m_Array; }

    // bool PushFront(const T&);
    // bool PushFront(T&&) noexcept;
    // bool PushFront(const Vector<T>&);
    // T PopFront();

    bool PushBack(const T&);
    bool PushBack(T&&) noexcept;
    bool PushBack(const Vector<T>&);
    T PopBack();

    bool Shift(int, bool=false);

    T& operator[] (unsigned int) const;

    class Iterator {
      public:
        T* current;

        Iterator(T& item): current(&item) {}

        T& operator*() { return *current; }
        Iterator& operator++() {
          if (current)
            current++;

          return *this;
        }

        Iterator operator++(int) {
          Iterator self = *this;
          (*this)++;

          return self;
        }

        bool operator==(Iterator const& other) { return (current == other.current); }
        bool operator!=(Iterator const& other) { return (current != other.current); }
    };

    Iterator begin() { return Iterator((*this)[0]); }
    Iterator end() { return Iterator((*this)[m_Size]); }

  private:
    const static unsigned int m_MaxSize = 2048;

    unsigned int m_Size, m_Capacity;
    T* m_Array;
};

}

#include "fpx_vector.hpp"

#endif
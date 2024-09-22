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

/**
 * PARTIALLY FINISHED AND WORKING
 */
template<typename T>
class Vector {
  public:
    /**
     * Creates a Vector object of type <T> of size 1.
     */
    Vector();

    /**
     * Creates a Vector object of type <T> of a given initial size.
     */
    Vector(unsigned int);
    /**
     * Creates a Vector out of a given array of type <T>.
     */
    Vector(T[]);

    /**
     * Destructor that frees any resources used.
     */
    ~Vector();

    /**
     * Returns the current amount of objects inside of the Vector
     */
    unsigned int GetSize() const { return m_Size; }

    /**
     * Returns the  current maximum capacity of the vector
     * (the amount of reserved space inside of it).
     */
    unsigned int GetCapacity() const { return m_Capacity; }

    /**
     * Returns the highest possible capacity, which the
     * Vector will not increase beyond.
     */
    static unsigned int MaxSize() { return m_MaxSize; }

    /**
     * Returns whether the vector is empty or not.
     */
    bool IsEmpty() const { return (m_Size == 0); }

    /**
     * Use this to double the current capacity.
     */
    bool DoubleCapacity();

    /**
     * Grow the current capacity by this amount.
     */
    bool Grow(const int& = 1);
    /**
     * Shrink the current capacity by this amount.
     */
    bool Shrink(const int& = 1);

    /**
     * Returns a reference to the first element in the vector.
     */
    T& Front() const { return m_Array[0]; }
    /**
     * Returns a reference to the last element in the vector
     */
    T& Back() const { return m_Array[m_Size-1]; }

    /**
     * Returns a pointer to the internal (heap-allocated) array.
     */
    T* Data() const { return m_Array; }

    // bool PushFront(const T&);
    // bool PushFront(T&&) noexcept;
    // bool PushFront(const Vector<T>&);
    // T PopFront();

    /**
     * Add an element to the back of the Vector.
     */
    bool PushBack(const T&);
    bool PushBack(T&&) noexcept;

    /**
     * Append another vector of the same type to the end
     * of the current vector.
     */
    bool PushBack(const Vector<T>&);
    T PopBack();

    /**
     * Shift all of the elements to the left by x spots.
     * 
     * Second argument:
     * "true" to remove elements that fall out,
     * "false" to cycle them back to the right.
     */
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
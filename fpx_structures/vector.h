#ifndef FPX_VECTOR_H
#define FPX_VECTOR_H

//
//  "vector.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "../fpx_types.h"

namespace fpx {

/**
 * PARTIALLY FINISHED AND WORKING
 */
template <typename T>
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
    Vector(T[], size_t length);

    /**
     * Destructor that frees any resources used.
     */
    ~Vector();

    /**
     * Returns the current amount of objects inside of the Vector
     */
    unsigned int GetSize() const {
      return m_Size;
    }

    /**
     * Returns the  current maximum capacity of the vector
     * (the amount of reserved space inside of it).
     */
    unsigned int GetCapacity() const {
      return m_Capacity;
    }

    /**
     * Returns the highest possible capacity, which the
     * Vector will not increase beyond.
     */
    static unsigned int MaxSize() {
      return m_MaxSize;
    }

    /**
     * Returns whether the vector is empty or not.
     */
    bool IsEmpty() const {
      return (m_Size == 0);
    }

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
    T& Front() const {
      return m_Array[0];
    }
    /**
     * Returns a reference to the last element in the vector
     */
    T& Back() const {
      return m_Array[m_Size - 1];
    }

    /**
     * Returns a pointer to the internal (heap-allocated) array.
     */
    T* Data() const {
      return m_Array;
    }

    // bool PushFront(const T&);
    // bool PushFront(T&&) noexcept;
    // bool PushFront(const Vector<T>&);

    /**
     * Return the first element of the vector, also removing it.
     * All the following elements will be shifted to compensate.
     */
    T PopFront();

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

    T Pop(unsigned int);

    /**
     * Shift all of the elements to the left by x spots.
     *
     * Second argument:
     * "true" to remove elements that fall out,
     * "false" to cycle them back to the right.
     */
    bool Shift(int, bool = false);

    T& operator[](unsigned int) const;

    class Iterator {
      public:
        T* current;

        Iterator(T* ptr) : current(ptr) { }

        T& operator*() {
          return *current;
        }
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

        bool operator==(Iterator const& other) {
          return (current == other.current);
        }
        bool operator!=(Iterator const& other) {
          return (current != other.current);
        }
    };

    Iterator begin() {
      return Iterator(m_Array);
    }
    Iterator end() {
      return Iterator(m_Array + m_Size);
    }

  private:
    const static unsigned int m_MaxSize = 2048;

    unsigned int m_Size, m_Capacity;
    T* m_Array;
};

}  // namespace fpx

#ifndef FPX_VECTOR_IMPL
#include "vector.hpp"
#endif  // FPX_VECTOR_IMPL

#endif  // FPX_VECTOR_H

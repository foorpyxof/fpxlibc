#ifndef FPX_VECTOR_HPP
#define FPX_VECTOR_HPP

//
//  "vector.hpp"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "../cpp-utils/exceptions.hpp"

#include "../fpx_macros.h"

#include <cmath>
#include <iostream>

namespace fpx {

/**
 * PARTIALLY FINISHED AND WORKING
 */
template <typename T> class Vector {
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
  bool Grow(const int & = 1);
  /**
   * Shrink the current capacity by this amount.
   */
  bool Shrink(const int & = 1);

  /**
   * Returns a reference to the first element in the vector.
   */
  T &Front() const { return m_Array[0]; }
  /**
   * Returns a reference to the last element in the vector
   */
  T &Back() const { return m_Array[m_Size - 1]; }

  /**
   * Returns a pointer to the internal (heap-allocated) array.
   */
  T *Data() const { return m_Array; }

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
  bool PushBack(const T &);
  bool PushBack(T &&) noexcept;

  /**
   * Append another vector of the same type to the end
   * of the current vector.
   */
  bool PushBack(const Vector<T> &);
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

  T &operator[](unsigned int) const;

  class Iterator {
  public:
    T *current;

    Iterator(T *ptr) : current(ptr) {}

    T &operator*() { return *current; }
    Iterator &operator++() {
      if (current)
        current++;

      return *this;
    }

    Iterator operator++(int) {
      Iterator self = *this;
      (*this)++;

      return self;
    }

    bool operator==(Iterator const &other) {
      return (current == other.current);
    }
    bool operator!=(Iterator const &other) {
      return (current != other.current);
    }
  };

  Iterator begin() { return Iterator(m_Array); }
  Iterator end() { return Iterator(m_Array + m_Size); }

private:
  const static unsigned int m_MaxSize = 2048;

  unsigned int m_Size, m_Capacity;
  T *m_Array;
};

template <typename T>
Vector<T>::Vector() : m_Size(0), m_Capacity(0), m_Array(new T[1]) {}

template <typename T>
Vector<T>::Vector(unsigned int len)
    : m_Size(0), m_Capacity(len), m_Array(new T[len + 1]) {}

template <typename T>
Vector<T>::Vector(T array[], size_t length)
    : m_Size(length), m_Capacity(length), m_Array(nullptr) {
  try {
    m_Array = new T[length];

    for (int i = 0; i < m_Capacity; i++)
      m_Array[i] = array[i];

  } catch (std::bad_alloc const &exc) {
    std::cout << exc.what() << std::endl;

    m_Size = 0;
    m_Capacity = 0;
    m_Array = new T[0];
  }
}

template <typename T> Vector<T>::~Vector() { delete[] m_Array; }

template <typename T> bool Vector<T>::DoubleCapacity() { return Grow(m_Size); }

template <typename T> bool Vector<T>::Grow(const int &more) {
  if (more < 0)
    return Shrink(0 - more);
  else if (more == 0)
    return true;

  T *newArray = nullptr;

  try {
    newArray = new T[m_Capacity + more];
  } catch (std::bad_alloc const &exc) {
    std::cout << exc.what() << std::endl;
    return false;
  }

  m_Capacity += more;

  // for(int i=0; i < m_Size; i++)
  //   newArray[i] = m_Array[i];

  // for(int i=m_Size; i < m_Capacity; i++)
  //   newArray[i] = 0;

  // delete[] m_Array;
  // m_Array = newArray;

  return true;
}

template <typename T> bool Vector<T>::Shrink(const int &less) {
  if (less < 0)
    return Grow(0 - less);
  else if (less == 0)
    return true;

  T *newArray = nullptr;

  if (!(m_Capacity - less))
    return false;

  try {
    newArray = new T[m_Capacity - less];
  } catch (std::bad_alloc const &exc) {
    std::cout << exc.what() << std::endl;
    return false;
  }

  m_Capacity -= less;
  m_Size = (m_Capacity < m_Size) ? m_Capacity : m_Size;

  for (int i = 0; i < m_Size; i++)
    newArray[i] = m_Array[i];

  for (int i = m_Size; i < m_Capacity; i++)
    newArray[i] = T();

  delete[] m_Array;
  m_Array = newArray;

  return true;
}

template <typename T> T Vector<T>::PopFront() {
  if (!m_Size)
    return T();
  T firstEle = m_Array[0];
  for (int i = 0; i < m_Size; i++) {
    m_Array[i] = (i == m_Size - 1) ? T() : m_Array[i + 1];
  }
  m_Size--;

  return firstEle;
}

template <typename T> T &Vector<T>::operator[](unsigned int index) const {
  if (index >= m_Size)
    throw IndexOutOfRangeException();
  return m_Array[index];
}

template <typename T> bool Vector<T>::PushBack(const T &item) {
  if (m_Size == m_Capacity)
    if (!Grow(4))
      return false;

  m_Array[m_Size] = item;
  m_Size++;

  return true;
}

template <typename T> bool Vector<T>::PushBack(T &&item) noexcept {
  if (m_Size == m_Capacity)
    if (!Grow(4))
      return false;

  m_Array[m_Size] = std::move(item);
  m_Size++;

  return true;
}

template <typename T> bool Vector<T>::PushBack(const Vector<T> &other) {
  const float x = other.GetSize() / 4.0f;
  if (m_Size + other.GetSize() > m_Capacity - 1)
    if (!Grow((int)ceil(x) * 4))
      return false;

  for (int i = 0; i < other.GetSize(); i++) {
    m_Array[m_Size + i] = other[i];
  }
  m_Size += other.GetSize();

  return true;
}

template <typename T> T Vector<T>::PopBack() {
  const T object = m_Array[m_Size - 1];

  m_Array[m_Size - 1] = 0;
  m_Size--;

  return object;
}

template <typename T> T Vector<T>::Pop(unsigned int index) {
  if (m_Size <= index)
    return T();
  T theEle = m_Array[index];
  for (int i = index; i < m_Size; i++) {
    m_Array[i] = (i == m_Size - 1) ? T() : m_Array[i + 1];
  }
  m_Size--;

  return theEle;
}

template <typename T> bool Vector<T>::Shift(int amount, bool remove) {
  UNUSED(remove);

  T old[m_Size];
  for (int i = 0; i < m_Size; i++)
    old[i] = m_Array[i];

  amount = -(amount % m_Size);
  if (!amount)
    return true;
  for (int i = 0; i < m_Size; i++)
    m_Array[i] = (i + amount < m_Size && i + amount > -1) ? old[i + amount]
                 : (i + amount > -1) ? old[i + amount - m_Size]
                                     : old[m_Size - (amount + i)];
  return true;
}

} // namespace fpx

#endif // FPX_VECTOR_HPP

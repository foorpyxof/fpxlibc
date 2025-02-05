#ifndef FPX_VECTOR_IMPL
#define FPX_VECTOR_IMPL

////////////////////////////////////////////////////////////////
//  "vector.hpp"                                              //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "vector.h"
#include "../fpx_cpp-utils/exceptions.h"

#include <iostream>
#include <cmath>

namespace fpx {

template<typename T>
Vector<T>::Vector():
  m_Size(0),
  m_Capacity(0),
  m_Array(new T[1])
  {}

template<typename T>
Vector<T>::Vector(unsigned int len):
  m_Size(0),
  m_Capacity(len),
  m_Array(new T[len+1])
  {}

template<typename T>
Vector<T>::Vector(T array[], size_t length):
  m_Size(length),
  m_Capacity(length),
  m_Array(nullptr)
  {
    try {
      m_Array = new T[length];

      for (int i=0; i < m_Capacity; i++)
        m_Array[i] = array[i];

    } catch (std::bad_alloc const& exc) {
      std::cout << exc.what() << std::endl;

      m_Size = 0;
      m_Capacity = 0;
      m_Array = new T[0];
    }
  }

template<typename T>
Vector<T>::~Vector() {
  delete[] m_Array;
}

template<typename T>
bool Vector<T>::DoubleCapacity() {
  return Grow(m_Size);
}

template<typename T>
bool Vector<T>::Grow(const int& more) {
  if (more < 0)
    return Shrink(0-more);
  else if (more == 0)
    return true;

  T* newArray = nullptr;
  
  try { newArray = new T[m_Capacity+more]; }
  catch(std::bad_alloc const& exc) {
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

template<typename T>
bool Vector<T>::Shrink(const int& less) {
  if (less < 0)
    return Grow(0-less);
  else if (less == 0)
    return true;

  T* newArray = nullptr;

  if(!(m_Capacity-less)) return false;

  try { newArray = new T[m_Capacity-less]; }
  catch (std::bad_alloc const& exc) {
    std::cout << exc.what() << std::endl;
    return false;
  }

  m_Capacity -= less;
  m_Size = (m_Capacity < m_Size) ? m_Capacity : m_Size;

  for(int i=0; i < m_Size; i++)
    newArray[i] = m_Array[i];

  for(int i=m_Size; i < m_Capacity; i++)
    newArray[i] = T();

  delete[] m_Array;
  m_Array = newArray;

  return true;
}

template<typename T>
T Vector<T>::PopFront() {
  if (!m_Size) return T();
  T firstEle = m_Array[0];
  for(int i=0; i<m_Size; i++) {
    m_Array[i] = (i == m_Size-1) ? T() : m_Array[i+1];
  }
  m_Size--;

  return firstEle;
}

template<typename T>
T& Vector<T>::operator[] (unsigned int index) const {
  if (index >= m_Size) throw IndexOutOfRangeException();
  return m_Array[index];
}

template<typename T>
bool Vector<T>::PushBack(const T& item) {
  if (m_Size == m_Capacity)
    if (!Grow(4)) return false;
  
  m_Array[m_Size] = item;
  m_Size++;

  return true;
}

template<typename T>
bool Vector<T>::PushBack(T&& item) noexcept {
  if (m_Size == m_Capacity)
    if (!Grow(4)) return false;
  
  m_Array[m_Size] = std::move(item);
  m_Size++;

  return true;
}

template<typename T>
bool Vector<T>::PushBack(const Vector<T>& other) {
  const float x = other.GetSize() / 4.0f;
  if (m_Size + other.GetSize() > m_Capacity-1)
    if (!Grow((int)ceil(x) * 4)) return false;

  for (int i = 0; i<other.GetSize(); i++) {
    m_Array[m_Size+i] = other[i];
  }
  m_Size += other.GetSize();
  
  return true;
}

template<typename T>
T Vector<T>::PopBack() {
  const T object = m_Array[m_Size-1];
  
  m_Array[m_Size-1] = 0;
  m_Size--;

  return object;
}

template<typename T>
T Vector<T>::Pop(unsigned int index) {
  if (m_Size <= index) return T();
  T theEle = m_Array[index];
  for(int i=index; i<m_Size; i++) {
    m_Array[i] = (i == m_Size-1) ? T() : m_Array[i+1];
  }
  m_Size--;

  return theEle;
}

template<typename T>
bool Vector<T>::Shift(int amount, bool remove) {
  T old[m_Size];
  for(int i=0; i<m_Size; i++)
    old[i] = m_Array[i];
  
  amount = -(amount%m_Size);
  if (!amount) return true;
  for (int i=0; i<m_Size; i++)
    m_Array[i] = (i + amount < m_Size && i + amount > -1) ? old[i + amount] : (i+amount > -1) ? old[i + amount - m_Size] : old[m_Size - (amount+i)];
  return true;
}

}

#endif // FPX_VECTOR_IMPL

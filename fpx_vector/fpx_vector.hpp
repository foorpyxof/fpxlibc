#include "fpx_vector.h"

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
Vector<T>::Vector(T array[]):
  m_Size(sizeof(array)/sizeof(T)),
  m_Capacity(sizeof(array)/sizeof(T)),
  m_Array(nullptr)
  {
    try {
      m_Array = new T[sizeof(array)/sizeof(T) + 1];

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
bool Vector<T>::DoubleCapacity() {
  return Grow(m_Size);
}

template<typename T>
bool Vector<T>::Grow(int more) {
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

  for(int i=0; i < m_Size; i++)
    newArray[i] = m_Array[i];

  for(int i=m_Size; i < m_Capacity; i++)
    newArray[i] = 0;

  delete[] m_Array;
  m_Array = newArray;

  return true;
}

template<typename T>
bool Vector<T>::Shrink(int less) {
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
    newArray[i] = 0;

  delete[] m_Array;
  m_Array = newArray;

  return true;
}

template<typename T>
T& Vector<T>::operator[] (unsigned int index) const {
  if (index > m_Size) throw new IndexOutOfRangeException;
  return m_Array[index];
}

template<typename T>
bool Vector<T>::PushBack(T item) {
  if (m_Size == m_Capacity)
    if (!Grow(4)) return false;
  
  m_Array[m_Size] = item;
  m_Size++;

  return true;
}

template<typename T>
T Vector<T>::PopBack() {
  const T object = m_Array[m_Size-1];
  
  m_Array[m_Size-1] = 0;
  m_Size--;

  return object;
}

}
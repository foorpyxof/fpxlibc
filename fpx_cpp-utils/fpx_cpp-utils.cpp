////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "fpx_cpp-utils.h"

namespace fpx {

  // method/constructor declarations for the GENERIC exception class
  Exception::Exception(int code): m_ErrMessage(FPX_GENERIC_ERRMSG), m_ErrCode(code) { }
  Exception::Exception(const char* message, int code): m_ErrMessage(message), m_ErrCode(code) { }

  int Exception::Code() const { return m_ErrCode; }
  const char* Exception::Message() const { return m_ErrMessage; }
  void Exception::Print() const { std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage << std::endl; }

  // method/constructor declarations for the NotImplementedException class
  NotImplementedException::NotImplementedException(int code): m_ErrMessage(FPX_NOTIMPLEMENTED_ERRMSG), m_ErrCode(code) { }
  NotImplementedException::NotImplementedException(const char* message, int code): m_ErrMessage(message), m_ErrCode(code) { }

  int NotImplementedException::Code() const { return m_ErrCode; }
  const char* NotImplementedException::Message() const { return m_ErrMessage; }
  void NotImplementedException::Print() const { std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage << std::endl; }

  // method/constructor declarations for the IndexOutOfRangeException class
  IndexOutOfRangeException::IndexOutOfRangeException(int code): m_ErrMessage(FPX_INDEXOUTOFRANGE_ERRMSG), m_ErrCode(code) { }
  IndexOutOfRangeException::IndexOutOfRangeException(const char* message, int code): m_ErrMessage(message), m_ErrCode(code) { }

  int IndexOutOfRangeException::Code() const { return m_ErrCode; }
  const char* IndexOutOfRangeException::Message() const { return m_ErrMessage; }
  void IndexOutOfRangeException::Print() const { std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage << std::endl; }

  // method/constructor declarations for the NetException class
  NetException::NetException(int code): m_ErrMessage(FPX_NET_ERRMSG), m_ErrCode(code) { }
  NetException::NetException(const char* message, int code): m_ErrMessage(message), m_ErrCode(code) { }

  int NetException::Code() const { return m_ErrCode; }
  const char* NetException::Message() const { return m_ErrMessage; }
  void NetException::Print() const { std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage << std::endl; }

  // method/constructor declarations for the ArgumentException class
  ArgumentException::ArgumentException(int code): m_ErrMessage(FPX_NET_ERRMSG), m_ErrCode(code) { }
  ArgumentException::ArgumentException(const char* message, int code): m_ErrMessage(message), m_ErrCode(code) { }

  int ArgumentException::Code() const { return m_ErrCode; }
  const char* ArgumentException::Message() const { return m_ErrMessage; }
  void ArgumentException::Print() const { std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage << std::endl; }

}
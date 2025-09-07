//
//  "exceptions.cpp"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

#include "cpp-utils/exceptions.hpp"

#include <iostream>

namespace fpx {

// method/constructor declarations for the GENERIC exception class
Exception::Exception(int code)
    : m_ErrCode(code), m_ErrMessage(FPX_GENERIC_ERRMSG) {}
Exception::Exception(const char *message, int code)
    : m_ErrCode(code), m_ErrMessage(message) {}

int Exception::Code() const { return m_ErrCode; }
const char *Exception::Message() const { return m_ErrMessage; }
void Exception::Print() const {
  std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage
            << std::endl;
}

// method/constructor declarations for the NotImplementedException class
NotImplementedException::NotImplementedException(int code)
    : m_ErrCode(code), m_ErrMessage(FPX_NOTIMPLEMENTED_ERRMSG) {}
NotImplementedException::NotImplementedException(const char *message, int code)
    : m_ErrCode(code), m_ErrMessage(message) {}

int NotImplementedException::Code() const { return m_ErrCode; }
const char *NotImplementedException::Message() const { return m_ErrMessage; }
void NotImplementedException::Print() const {
  std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage
            << std::endl;
}

// method/constructor declarations for the IndexOutOfRangeException class
IndexOutOfRangeException::IndexOutOfRangeException(int code)
    : m_ErrCode(code), m_ErrMessage(FPX_INDEXOUTOFRANGE_ERRMSG) {}
IndexOutOfRangeException::IndexOutOfRangeException(const char *message,
                                                   int code)
    : m_ErrCode(code), m_ErrMessage(message) {}

int IndexOutOfRangeException::Code() const { return m_ErrCode; }
const char *IndexOutOfRangeException::Message() const { return m_ErrMessage; }
void IndexOutOfRangeException::Print() const {
  std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage
            << std::endl;
}

// method/constructor declarations for the NetException class
NetException::NetException(int code)
    : m_ErrCode(code), m_ErrMessage(FPX_NET_ERRMSG) {}
NetException::NetException(const char *message, int code)
    : m_ErrCode(code), m_ErrMessage(message) {}

int NetException::Code() const { return m_ErrCode; }
const char *NetException::Message() const { return m_ErrMessage; }
void NetException::Print() const {
  std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage
            << std::endl;
}

// method/constructor declarations for the ArgumentException class
ArgumentException::ArgumentException(int code)
    : m_ErrCode(code), m_ErrMessage(FPX_NET_ERRMSG) {}
ArgumentException::ArgumentException(const char *message, int code)
    : m_ErrCode(code), m_ErrMessage(message) {}

int ArgumentException::Code() const { return m_ErrCode; }
const char *ArgumentException::Message() const { return m_ErrMessage; }
void ArgumentException::Print() const {
  std::cout << "Error " << this->m_ErrCode << ": " << this->m_ErrMessage
            << std::endl;
}

} // namespace fpx

#ifndef FPX_CPPUTILS_H
#define FPX_CPPUTILS_H

////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include <iostream>

#define FPX_GENERIC_ERRMSG "An exception has occured at runtime!"
#define FPX_NOTIMPLEMENTED_ERRMSG "This functionality has not been implemented yet!"
#define FPX_INDEXOUTOFRANGE_ERRMSG "The index you tried to reach is not in range!"
#define FPX_NET_ERRMSG "A NetException has occured!"
#define FPX_ARG_ERRMSG "An ArgumentException has occured!"

namespace fpx {

  // Custom exceptions declared below
  class Exception {
    public:
      Exception(int);
      Exception(const char* = FPX_GENERIC_ERRMSG, int = -1);

      virtual int Code() const;
      virtual const char* Message() const;
      virtual void Print() const;

    protected:
      int m_ErrCode;
      const char* m_ErrMessage;
  };

  class NotImplementedException : public Exception {
    public:
      NotImplementedException(int);
      NotImplementedException(const char* = FPX_NOTIMPLEMENTED_ERRMSG, int = -2);

      int Code() const;
      const char* Message() const;
      void Print() const;

    private:
      int m_ErrCode;
      const char* m_ErrMessage;
  };

  class IndexOutOfRangeException : public Exception {
    public:
      IndexOutOfRangeException(int);
      IndexOutOfRangeException(const char* = FPX_INDEXOUTOFRANGE_ERRMSG, int = -3);

      int Code() const;
      const char* Message() const;
      void Print() const;

    private:
      int m_ErrCode;
      const char* m_ErrMessage;
  };

  class NetException : public Exception {
    public:
      NetException(int);
      NetException(const char* = FPX_NET_ERRMSG, int = -4);

      int Code() const;
      const char* Message() const;
      void Print() const;

    private:
      int m_ErrCode;
      const char* m_ErrMessage;
  };

  class ArgumentException : public Exception {
    public:
      ArgumentException(int);
      ArgumentException(const char* = FPX_ARG_ERRMSG, int = -4);

      int Code() const;
      const char* Message() const;
      void Print() const;

    private:
      int m_ErrCode;
      const char* m_ErrMessage;
  };

}

#endif /* FPX_CPPUTILS_H */
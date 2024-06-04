#ifndef FPX_SERVER_H
#define FPX_SERVER_H

#include "../../fpx_cpp-utils/fpx_cpp-utils.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fpx {

class TcpServer {
  public:
    TcpServer(unsigned short);
    ~TcpServer();
  public:
    void Listen();
    void Close();

  private:
    unsigned short m_Port;

    int m_Socket4;
    // int m_Socket6;
    
    struct sockaddr_in m_SocketAddress4;
    // struct sockaddr_in6 m_SocketAddress6;
  private:
    bool pvt_DropConnection();

};

}

#endif // FPX_SERVER_H
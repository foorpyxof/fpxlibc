#include "fpx_server.h"

namespace fpx {

TcpServer::TcpServer(unsigned short port = 0):
  m_Port(port),
  m_Socket4(0),
  m_SocketAddress4 { AF_INET, htons(port), { INADDR_ANY } }
  {}

TcpServer::~TcpServer() {}

void TcpServer::Listen() {
  m_Socket4 = socket(AF_INET, SOCK_STREAM, 0);
  if ( m_Socket4 < 0 ) {
    throw NetException("Failed to create socket.");
  }

  if ( bind(m_Socket4, (sockaddr*) &m_SocketAddress4, sizeof(m_SocketAddress4)) < 0 ) {
    throw NetException("Failed to bind to socket.");
  }

  if ( listen(m_Socket4, 32) < 0 ) {
    throw NetException("Socket failed to listen.");
  }

  std::ostream* stream;
  *stream << "\n\
  Listening on " << inet_ntoa(m_SocketAddress4.sin_addr) << ':' <<
  ntohs(m_SocketAddress4.sin_port) << "\n\n";

  return;
}

void TcpServer::Close() {
  close(m_Socket4);
  return;
}

}
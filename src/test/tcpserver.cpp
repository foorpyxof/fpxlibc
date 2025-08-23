#include "networking/tcp/tcpserver.hpp"
#include "cpp-utils/exceptions.hpp"
#include "test/test-definitions.hpp"

using namespace fpx;

int main() {

  TcpServer tcpServ;

  try {
    tcpServ.Listen("0.0.0.0", 7777);
  } catch (NetException& exc) { exc.Print(); }
}

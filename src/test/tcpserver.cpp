#include "../fpx_networking/tcp/tcpserver.h"
#include "../fpx_cpp-utils/exceptions.h"
#include "test-definitions.h"

using namespace fpx;

int main() {

  TcpServer tcpServ;

  try {
    tcpServ.Listen("0.0.0.0", 7777);
  } catch (NetException& exc) { exc.Print(); }
}

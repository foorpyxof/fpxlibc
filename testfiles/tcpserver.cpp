#include "test-definitions.h"
#include "fpx_networking/tcp/tcpserver.h"

using namespace fpx;

int main() {

  TcpServer tcpServ("0.0.0.0", 7777);

  try {
    tcpServ.Listen();
  } catch (NetException& exc) {
    exc.Print();
  }

}
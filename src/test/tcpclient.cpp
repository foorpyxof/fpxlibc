#include "../fpx_cpp-utils/exceptions.h"
#include "../fpx_networking/tcp/tcpclient.h"
#include "test-definitions.h"
extern "C" {
  #include "../fpx_string/string.h"
}

using namespace fpx;

void ReadCallback(uint8_t* theBytes) {
  printf("\nNew message:\n");
  for (int i = 0; i < fpx_getstringlength((char*)theBytes); i++) {
    printf("%02x", (theBytes)[i]);
  }
  printf("\nMessage over.\n");
}

int main() {

  TcpClient tcpClient("127.0.0.1", 7777);
  bool background = false;
  try {
    // if string is empty, username is 'Anonymous'.
    // Also, a maximum of 16 characters is enforced by both the server and this specific client.
    tcpClient.Connect(
      (background) ? TcpClient::Mode::Background : TcpClient::Mode::Interactive, ReadCallback);
  } catch (Exception& exc) { exc.Print(); }

  // simple way to send messages when Mode::Background is selected
  char sendbuf[32];
  while (background) {
    memset(sendbuf, 0, 32);
    fgets(sendbuf, sizeof(sendbuf), stdin);
    tcpClient.SendRaw(sendbuf);
  }
}

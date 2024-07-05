#ifndef FPX_CLIENT_H
#define FPX_CLIENT_H

#include "../../fpx_cpp-utils/fpx_cpp-utils.h"
extern "C"{
#include "../../fpx_string/fpx_string.h"
}

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

#define BUF_SIZE 1024
#define DEFAULTPORT 8080

#define FPX_INCOMING "MSG:"
#define FPX_DISCONNECT "DISCONNECT"
#define FPX_INIT "NAME:"

namespace fpx {

typedef void (*fn_ptr)(char*);

class TcpClient {
  public:
    enum class Mode {
      Interactive,
      NonInteractive
    };
  
  public:
    // TcpClient();
    // TcpClient(const char*, short = DEFAULTPORT);
    // ~TcpClient();
    static bool Setup(const char* ip, short port = DEFAULTPORT);
    static void Connect(Mode mode, void (*readerCallback)(char*), const char* name = "");
    static bool Disconnect();
    static void SendRaw(const char*);
    static void SendMessage(const char*);
  
  private:
    struct threaddata {
      fn_ptr fn;
    };

    static struct threaddata m_ThreadData;
    static pthread_t m_ReaderThread, m_WriterThread;

    static const char* m_SrvIp;
    static short m_SrvPort;

    static struct sockaddr_in m_SrvAddress;

    static int m_Socket;

    static char m_ReadBuffer[];
    static char m_WriteBuffer[];
    static char m_Input[];
    
  private:
    static void* pvt_ReaderLoop(void*);
    static void* pvt_WriterLoop(void*);


};

}

#endif // FPX_CLIENT_H
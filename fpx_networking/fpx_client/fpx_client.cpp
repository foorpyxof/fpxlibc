#include "fpx_client.h"

namespace fpx {

// TcpClient::TcpClient():
//   m_SrvIp("0.0.0.0"), m_SrvPort(DEFAULTPORT),
//   m_Socket(-1),
//   m_SrvAddress { AF_INET, htons(m_SrvPort), {  } },
//   m_WriterThread(0), m_ReaderThread(0)
//   {
//     inet_aton(m_SrvIp, &m_SrvAddress.sin_addr);
//   }

// TcpClient::TcpClient(const char* ip, short port):
//   m_SrvIp(ip), m_SrvPort(port),
//   m_Socket(-1),
//   m_SrvAddress { AF_INET, htons(m_SrvPort), {  } },
//   m_WriterThread(0), m_ReaderThread(0)
//   {
//     inet_aton(m_SrvIp, &m_SrvAddress.sin_addr);
//   }

// TcpClient::~TcpClient() {
//   pvt_GracefulQuit();
// }

// STATIC DECLARATIONS
struct TcpClient::threaddata TcpClient::m_ThreadData;
pthread_t TcpClient::m_ReaderThread, TcpClient::m_WriterThread;

const char* TcpClient::m_SrvIp;
short TcpClient::m_SrvPort = 0;

struct sockaddr_in TcpClient::m_SrvAddress;

int TcpClient::m_Socket;

char TcpClient::m_ReadBuffer[BUF_SIZE];
char TcpClient::m_WriteBuffer[BUF_SIZE];
char TcpClient::m_Input[BUF_SIZE-16];
// END STATIC DECLARATIONS

bool TcpClient::Setup(const char* ip, short port) {
  m_SrvIp = ip, m_SrvPort = port;
  m_Socket = -1;
  m_ReaderThread = 0, m_WriterThread = 0;
  m_SrvAddress = { AF_INET, htons(m_SrvPort), {  } };
  memset(&m_ThreadData, 0, sizeof(m_ThreadData));
  return inet_aton(m_SrvIp, &m_SrvAddress.sin_addr);
}

void TcpClient::Connect(Mode mode, void (*readerCallback)(char*), const char* name) {
  if (m_SrvPort == 0)
    throw NetException("Client not set up. Run Setup()");
  if (mode == Mode::NonInteractive && readerCallback == nullptr)
    throw fpx::ArgumentException("No callback function was supplied.");
  if (mode == Mode::NonInteractive)
    m_ThreadData.fn = readerCallback;
  
  m_Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(m_Socket, (struct sockaddr*)&m_SrvAddress, sizeof(m_SrvAddress)) == -1) {
    printf("\nInvalid address or address not supported\n");
    return;
  }

  pthread_create(&m_ReaderThread, NULL, pvt_ReaderLoop, NULL);
  pthread_create(&m_WriterThread, NULL, pvt_WriterLoop, (void*)name);

  pthread_join(m_WriterThread, NULL);
  pthread_kill(m_ReaderThread, SIGINT);

  return;
}


bool TcpClient::Disconnect() {
  write(m_Socket, FPX_DISCONNECT, fpx_getstringlength(FPX_DISCONNECT));
  return !(close(m_Socket));
}

void TcpClient::SendRaw(const char* msg) {
  snprintf(m_WriteBuffer, BUF_SIZE, "%s", msg);
  write(m_Socket, m_WriteBuffer, fpx_getstringlength(m_WriteBuffer));
  memset(m_WriteBuffer, 0, BUF_SIZE);
}

void TcpClient::SendMessage(const char* msg) {
  char buf[BUF_SIZE-16];
  snprintf(buf, sizeof(buf), "%s%s", FPX_INCOMING, msg);
  SendRaw(buf);
}

void* TcpClient::pvt_ReaderLoop(void*) {
  memset(&m_ReadBuffer, 0, BUF_SIZE);

  if (m_ThreadData.fn) {
    while (1) {
      if (read(m_Socket, m_ReadBuffer, BUF_SIZE) == -1) {
        //handle errors
      }
      if (!(short)m_ReadBuffer[0]) {
        //code 1: server closed connection
        m_ThreadData.fn((char*)"CONN_CLOSE");
      }
      m_ReadBuffer[strcspn(m_ReadBuffer, "\r\n")] = 0;
      m_ThreadData.fn(m_ReadBuffer);
      memset(m_ReadBuffer, 0, BUF_SIZE);
    }
  } else {
    while (1) {
      if (read(m_Socket, m_ReadBuffer, BUF_SIZE) == -1) {
        // handle read error
      }
      if (!(short)m_ReadBuffer[0]) {
        printf("\nServer closed connection.\n");
        pthread_kill(m_WriterThread, SIGINT);
        pthread_exit(NULL);
      }
      if (m_ReadBuffer[strlen(m_ReadBuffer)-1] != '\n')
        printf("\r%s\n>> ", m_ReadBuffer);
      else
        printf("\r%s>> ", m_ReadBuffer);
      fflush(stdout);
      memset(m_ReadBuffer, 0, BUF_SIZE);
    }
  }
}

void* TcpClient::pvt_WriterLoop(void* arg) {
  bool preventPrompt = 0;
  const char* name = (const char*)arg;
  memset(&m_WriteBuffer, 0, BUF_SIZE);

  sprintf(m_WriteBuffer, "%s", FPX_INIT);
  strcat(m_WriteBuffer, name);
  write(m_Socket, m_WriteBuffer, strlen(m_WriteBuffer));

  while (1) {
    memset(m_Input, 0, sizeof(m_Input));
    memset(m_WriteBuffer, 0, BUF_SIZE);
    if (!preventPrompt)
      printf(">> "); fflush(stdout);
    preventPrompt = 0;
    if ((fgets(m_Input, sizeof(m_Input), stdin) != NULL && strncmp(m_Input, "quit", 4))) {
      m_Input[strcspn(m_Input, "\r\n")] = 0;
      if (!(*m_Input)) {
        continue;
      }
      if (*m_Input == '!' && *(m_Input+1) != 0)
        preventPrompt = 1;
      SendMessage(m_Input);
    } else {
      Disconnect();
      printf("Quitting...\n");
      pthread_exit(NULL);
    }

  }
}

}
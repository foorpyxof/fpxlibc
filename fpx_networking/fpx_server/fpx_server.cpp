////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "fpx_server.h"

namespace fpx {

// STATIC DECLARATIONS
unsigned short TcpServer::m_Port;

int TcpServer::m_Socket4;

struct pollfd TcpServer::m_Sockets[];
struct TcpServer::client TcpServer::m_Clients[];

struct sockaddr_in TcpServer::m_SocketAddress4;

struct sockaddr TcpServer::m_ClientAddress;
socklen_t TcpServer::m_ClientAddressSize;

short TcpServer::m_ConnectedClients;

bool TcpServer::m_IsListening;

pthread_t TcpServer::m_AcceptThread;
// END STATIC DECLARATIONS

bool TcpServer::Setup(const char* ip, unsigned short port) {
  if (m_IsListening) {
    throw NetException("Server is listening.");
  }
  m_Port = port;
  m_Socket4 = 0;
  m_SocketAddress4 = { AF_INET, htons(port), {  } };

  m_IsListening = false;
  
  m_ConnectedClients = 0;

  memset(m_Sockets, -1, sizeof(m_Sockets));
  memset(m_Clients, 0, sizeof(m_Clients));
  return inet_aton(ip, &(m_SocketAddress4.sin_addr));
}

void TcpServer::Listen() {
  int optvalTrue = 1;

  char readBuffer[BUF_SIZE];
  char writeBuffer[BUF_SIZE];
  memset(readBuffer, 0, BUF_SIZE);
  memset(writeBuffer, 0, BUF_SIZE);

  m_Socket4 = socket(AF_INET, SOCK_STREAM, 0);
  if (m_Socket4 < 0) {
    Close();
    throw NetException("Failed to create socket.");
  }

  setsockopt(m_Socket4, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optvalTrue, sizeof(optvalTrue));

  if (bind(m_Socket4, (sockaddr*) &m_SocketAddress4, sizeof(m_SocketAddress4)) < 0) {
    Close();
    throw NetException("Failed to bind to socket.");
  }

  if (listen(m_Socket4, 16) < 0) {
    Close();
    throw NetException("Socket failed to listen.");
  }

  m_Sockets[0].fd = m_Socket4;
  m_Sockets[0].events = POLLIN;

  printf("TCPserver listening on %s:%d\n", inet_ntoa(m_SocketAddress4.sin_addr), ntohs(m_SocketAddress4.sin_port));
  
  int threadCreation = pthread_create(&m_AcceptThread, NULL, pvt_AcceptLoop, NULL);
  switch (threadCreation) {
    case 0:
      //thread started successfully
      break;

    default:
      //thread did not start successfully
      break;
  }

  m_IsListening = true;

  while (m_IsListening) {

    // printf("Polling sockets for %d seconds or until a socket is ready...\n", POLLTIMEOUT/1000);
    int result = poll(m_Sockets, sizeof(m_Sockets)/sizeof(m_Sockets[0]), POLLTIMEOUT);

    if (result == -1) {
      printf("An error occured while polling sockets. %s\n", strerror(errno));
      continue;
    }
    if (result == 0) {
      //poll timed out
      continue;
    }

    short j = 0;
    for(short i = 1; i<MAX_CONNECTIONS+1 && j<result; i++) {

      bool handled = 0;
      if (m_Sockets[i].revents & POLLIN) {
        //ready to receive from device

        ssize_t bytesRead = read(m_Sockets[i].fd, readBuffer, BUF_SIZE);
        if (bytesRead > 0) {
          if (!strncmp(readBuffer, FPX_INCOMING, strlen(FPX_INCOMING))) {
            char* cleanMsg = (char*)fpx_substr_replace(readBuffer, FPX_INCOMING, "");
            cleanMsg[strcspn(cleanMsg, "\r\n")] = 0;
            switch (*cleanMsg) {
              case '!':
                char temp[40];
                if (!strcmp(cleanMsg, "!online")) {
                  memset(temp, 0, sizeof(temp));
                  sprintf(writeBuffer, "\nHere is a list of connected clients (%d/%d):\n\n", m_ConnectedClients, MAX_CONNECTIONS);
                  short k=1;
                  for (client& cnt : m_Clients) {
                    if (*cnt.Name != 0) {
                      sprintf(temp, "%s (%d)", cnt.Name, k);
                      if (k==i)
                        strcat(temp, " << YOU");
                      strcat(temp, "\n");
                      strcat(writeBuffer, temp);
                      memset(temp, 0, sizeof(temp));
                    }
                    k++;
                  }
                  strcat(writeBuffer, "\n");
                } else
                if (!strcmp(cleanMsg, "!whoami")) {
                  sprintf(temp, "\nYou are: %s (%d)\n\n", m_Clients[i-1].Name, i);
                  strcat(writeBuffer, temp);
                  memset(temp, 0, sizeof(temp));
                } else
                if (!strcmp(cleanMsg, "!help")) {
                  sprintf(writeBuffer, "\nHere is a list of commands:\n!whoami\n!online\n!pm [id] [message]\n\n");
                } else
                if (!strcmp(cleanMsg, "!id")) {
                  sprintf(writeBuffer, "%d", i);
                } else
                if (!strncmp(cleanMsg, "!pm", 3)) {
                  // TODO:  improve this command's code.
                  //        wrote this while very tired and braindead lol
                  //        - Erynn
                  if (!(cleanMsg[3] == ' '))
                    sprintf(writeBuffer, "Bad syntax: '!pm'. Requires '!pm [id] [message]'");
                  else {
                    const char* args = fpx_substr_replace(cleanMsg, "!pm ", "");
                    const char* argsCpy = args;
                    char recIDchars[4], message[BUF_SIZE], messageCpy[BUF_SIZE];
                    int recID;

                    memcpy(recIDchars, args, (strspn(args, "0123456789") > 4) ? 4 : strspn(args, "0123456789"));
                    recID = atoi(recIDchars);
                    args += strspn(args, "0123456789");
                    if (*args != ' ')
                      sprintf(writeBuffer, "Bad syntax: '!pm'. Requires '!pm [id] [message]'");
                    else {
                      args++;
                      sprintf(message, "Private from [%s (%d)]: ", m_Clients[i-1].Name, i);
                      memcpy(messageCpy, message, BUF_SIZE);
                      snprintf(message, BUF_SIZE, "%s%s\n", messageCpy, args);
                      write(m_Sockets[recID].fd, message, strlen(message));
                      free((char*)argsCpy);
                    }
                  }
                } else {
                  sprintf(writeBuffer, "\nUnrecognised command '%s'. Try '!help' for a list of commands.\n\n", cleanMsg);
                }
                if (*writeBuffer != 0) {
                  write(m_Sockets[i].fd, writeBuffer, strlen(writeBuffer));
                  memset(writeBuffer, 0, strlen(writeBuffer));
                }
                break;
            case 0:
              break;
            default:
              sprintf(writeBuffer, "[%s (%d)]: %s\n", m_Clients[i-1].Name, i, cleanMsg);
              printf(writeBuffer);
              for(pollfd& socket : m_Sockets) {
                if (!(socket.fd == m_Socket4 || socket.fd == m_Sockets[i].fd))
                  write(socket.fd, writeBuffer, strlen(writeBuffer));
              }
              memset(writeBuffer, 0, strlen(writeBuffer));
              break;
            }
            free((char*)cleanMsg);
          }

          if(!strncmp(readBuffer, FPX_DISCONNECT, strlen(FPX_DISCONNECT))) {
            pvt_HandleDisconnect(m_Sockets[i], i);
            j+=1;
            memset(readBuffer, 0, BUF_SIZE);
            continue;
          } else

          if (!strncmp(readBuffer, FPX_INIT, strlen(FPX_INIT))) {
            char* clientName = (char*)fpx_substr_replace(readBuffer, FPX_INIT, "");
            clientName[strcspn(clientName, "\r\n")] = 0;
            if (!(*clientName == 0)) {
              memset(m_Clients[i-1].Name, 0, sizeof(m_Clients[i-1]));
              memcpy(m_Clients[i-1].Name, clientName, sizeof(m_Clients[i-1]));
            }
            free(clientName);
          } else
          if (!strncmp(readBuffer, FPX_ECHO, strlen(FPX_ECHO))) {
            const char* cleanMsg = fpx_substr_replace(readBuffer, FPX_ECHO, "");
            snprintf(writeBuffer, BUF_SIZE, "%s", cleanMsg);
            write(m_Sockets[i].fd, writeBuffer, strlen(writeBuffer));
            memset(writeBuffer, 0, BUF_SIZE);
          }

          memset(readBuffer, 0, BUF_SIZE);
          handled = 1;

        } else {
          pvt_HandleDisconnect(m_Sockets[i], i);
          j+=1;
          continue;
        }
      }
      if (m_Sockets[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
        //device hung up
        pvt_HandleDisconnect(m_Sockets[i], i);
        j+=1;
        continue;
      }
      if (m_Sockets[i].revents & POLLOUT) {
        //device ready to be written to
        // handled = 1;
      }
      j+=handled;
    }

  }

  pthread_kill(m_AcceptThread, SIGTERM);

  return;
}

void TcpServer::ListenSecure(const char* keypath, const char* certpath) {
  throw NotImplementedException("SSL/TLS is not yet implemented.");
}

void TcpServer::Close() {
  close(m_Socket4);
  return;
}

void* TcpServer::pvt_AcceptLoop(void*) {
  const char welcomeMessage[] = "Welcome to fpx_TCP! :)\nSend '!help' for a list of commands.\n\n";
  while (1) {
    if (m_ConnectedClients < MAX_CONNECTIONS) {
      int client = accept(m_Socket4, &m_ClientAddress, &m_ClientAddressSize);
      if (client < 0) {
        close(client);
        continue;
      }
      short i = -1;
      for (pollfd& socket : m_Sockets) {
        if (socket.fd == -1) {
          socket = { client, POLLIN, 0 };
          m_Clients[i] = { "Anonymous" };
          m_ConnectedClients++;
          printf("A new client (%d) has connected!\n", i+1);
          write(client, welcomeMessage, sizeof(welcomeMessage));
          break;
        }
        i++;
      }
    }
  }
  return NULL;
}

bool TcpServer::pvt_DropConnection() {
  return 0;
}

void TcpServer::pvt_HandleDisconnect(pollfd& client, short& clientNumber) {
  m_ConnectedClients--;
  close(client.fd);
  printf("[%s (%d)] disconnected.\n", m_Clients[clientNumber-1].Name, clientNumber);
  client = { -1, -1, 0 };
  memset(&m_Clients[clientNumber-1], 0, sizeof(m_Clients[clientNumber-1]));
}

}
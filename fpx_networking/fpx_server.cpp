////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "fpx_server.h"

// loose functions declared here
namespace fpx::ServerProperties {

void* TcpAcceptLoop(void* arguments) {
  tcp_acceptargs_t* args = (tcp_acceptargs_t*)arguments;
  const char welcomeMessage[] = "Welcome to fpx_TCP! :)\nSend '!help' for a list of commands.\n\n";
  while (1) {
    if (*(args->ClientCountPtr) < FPX_MAX_CONNECTIONS) {
      int client = accept(*(args->ListenSocketPtr), args->ClientAddressBlockPtr, args->ClientAddressSizePtr);
      if (client < 0) {
        close(client);
        continue;
      }
      for (short i = 1; i < FPX_MAX_CONNECTIONS+1; i++) {
        pollfd& socket = args->ConnectedSockets[i];
        if (socket.fd == -1) {
          socket = { client, POLLIN, 0 };
          args->ConnectedClients[i-1] = { "Anonymous" };
          *(args->ClientCountPtr)++;
          printf("A new client (%d) has connected!\n", i);
          write(client, welcomeMessage, sizeof(welcomeMessage));
          break;
        }
      }
    }
  }
  return NULL;
}

void* HttpProcessingThread(void* threadpack) {
  // printf("An HTTP-processing thread has started!\n");
  HttpServer::http_threadpackage_t* package = (HttpServer::http_threadpackage_t*)threadpack;

  char readBuffer[FPX_HTTP_READ_BUF+1];
  char writeBuffer[FPX_HTTP_WRITE_BUF];
  // char headWriteBuffer[FPX_HTTP_WRITE_BUF*(1/4)];
  // char bodyWriteBuffer[FPX_HTTP_WRITE_BUF*(3/4)];

  while (1) {
    pthread_mutex_lock(&package->TalkingStick);
    pthread_cond_wait(&package->Condition, &package->TalkingStick);
    
    char method[8];
    long totalWritten;
    char closeWhenDone = 0;

    HttpServer::http_endpoint_t* endpointPtr = nullptr;
    pollfd client = { package->ClientFD, POLLIN, 0 };
    while(1) {
      int result = poll(&client, 1, 7000);
      if (result == 0)
        break;
      else
      if (result == -1)
        perror("Could not poll client socket");
            
      ssize_t amountRead = recv(package->ClientFD, readBuffer, FPX_HTTP_READ_BUF+1, 0);
      if (!(amountRead)) break;
      HttpServer::http_request_t request;
      HttpServer::http_response_t response;
      
      HttpServer::http_response_t tempResponse = {  };
      

      if (amountRead == FPX_HTTP_READ_BUF+1) {
        tempResponse = package->Caller->Response413;
        response = tempResponse;
        closeWhenDone = 1;
        goto buildFromPreset;
      }

      int firstLineRead, headerLength;
      sscanf(readBuffer, "%s %255s %15s\r\n%n", method, request.URI, request.Version, &firstLineRead);
      for(int i = firstLineRead; i < 1024+firstLineRead;) {
        int newRead;
        char tempBuf[256];
        if (sscanf(&readBuffer[i], "%255[^\r\n]\r\n%n", tempBuf, &newRead) > 0) {
          if (1024 - strlen(request.Headers) < newRead) break; //header section too large
          memcpy(request.Headers+(i-firstLineRead), &readBuffer[i], newRead);
          memset(tempBuf, 0, sizeof(tempBuf));
          i += newRead;
          headerLength += newRead;
        } else break;
      }
      memset(request.Headers+(headerLength-2), 0, 2);

      int bodyRead;
      sscanf(&readBuffer[firstLineRead+fpx_getstringlength(request.Headers)+2], "%2799s%n", request.Body, &bodyRead);
      request.Method = HttpServer::ParseMethod(method);

      printf("Serving '%s' to client.\n", request.URI);

      if ((strcmp(request.Version, "HTTP/1.1"))) {
        tempResponse = package->Caller->Response505;
        response = tempResponse;
      }
      else {
        if (!endpointPtr) {
          for (short i=0; (!endpointPtr) && i<FPX_HTTP_ENDPOINTS; i++) {
            if (!strcmp(package->Endpoints[i].URI, request.URI)) {
              endpointPtr = &package->Endpoints[i];
              break;
            }
          }
        }
        if (endpointPtr && (request.Method & endpointPtr->AllowedMethods)) {
          endpointPtr->Callback(&request, &response);
        } else {
          if (endpointPtr && !(request.Method & endpointPtr->AllowedMethods)) {
            tempResponse = package->Caller->Response405;
          } else {
            // 404 not found
            tempResponse = package->Caller->Response404;
          }
          response = tempResponse;
        }
      }
      
      buildFromPreset:

      sprintf(response.Version, "HTTP/1.1");

      if (tempResponse.Headers) {
        response.Headers = (char*)malloc(tempResponse.GetHeaderLength());
        // printf("response.Headers: 0x%x\n", response.Headers);
        memcpy(response.Headers, tempResponse.Headers, tempResponse.GetHeaderLength());
      }
      if (tempResponse.Body) {
        response.Body = (char*)malloc(tempResponse.GetBodyLength());
        // printf("response.Body: 0x%x\n", response.Body);
        memcpy(response.Body, tempResponse.Body, tempResponse.GetBodyLength());
      }

      char* connectionHeader = request.GetHeaderValue("Connection");

      // printf("Checking keepalive\n");
      if (!strcmp(connectionHeader, "close")) {
        // printf("Found 'close'\n");
        closeWhenDone = 1;
        response.AddHeader("Connection: close");
      }
      else {
        // printf("Found 'keep-alive'\n");
        response.AddHeader("Connection: keep-alive");
        response.AddHeader("Keep-Alive: timeout=7");
      }

      if (strcmp(connectionHeader, "Header not found")) free(connectionHeader); 


      short amountWritten;
      char tempHeaderBuf[256];
      sprintf(tempHeaderBuf, "Content-Length: %d", response.GetBodyLength());
      response.AddHeader(tempHeaderBuf);
      memset(tempHeaderBuf, 0, 256);

      if (request.Method & HttpServer::HttpMethod::HEAD) {
        amountWritten = snprintf(writeBuffer, FPX_HTTP_WRITE_BUF, "%s %s %s\r\n%s%s\r\n",
          response.Version,
          response.Code,
          response.Status,
          package->Caller->GetDefaultHeaders(),
          response.Headers
        );
      } else {
        amountWritten = snprintf(writeBuffer, FPX_HTTP_WRITE_BUF, "%s %s %s\r\n%s%s\r\n%s",
          response.Version,
          response.Code,
          response.Status,
          package->Caller->GetDefaultHeaders(),
          response.Headers,
          response.Body
        );
      }

      if (response.Headers) free(response.Headers);
      if (response.Body) free(response.Body);
      memset(&request, 0, sizeof(request));
      memset(&response, 0, sizeof(response));
      totalWritten += amountWritten;

      send(package->ClientFD, writeBuffer, totalWritten, 0);
      endpointPtr = nullptr;

      memset(readBuffer, 0, sizeof(readBuffer));
      memset(writeBuffer, 0, sizeof(writeBuffer));
      
      if (closeWhenDone) break;
    }

    close(package->ClientFD);

    pthread_mutex_unlock(&package->TalkingStick);
  }

  return NULL;
}

}

// TCP server class
namespace fpx {

TcpServer::TcpServer(const char* ip, unsigned short port):
m_Port(port), m_Socket4(0),
m_SocketAddress4{ AF_INET, htons(port), {  } },
m_ClientAddressSize(sizeof(struct sockaddr)),
m_ConnectedClients(0),
m_IsListening(false),
m_AcceptThread(0),
m_Sockets{ -1 }, m_Clients{ 0 }
{
  memset(m_Sockets, -1, sizeof(m_Sockets));
  memset(m_Clients, 0, sizeof(m_Clients));
  if (!inet_aton(ip, &(m_SocketAddress4.sin_addr))) perror("Invalid IP address");
}

void TcpServer::Listen() {
  int optvalTrue = 1;

  char readBuffer[FPX_BUF_SIZE] = { 0 };
  char writeBuffer[FPX_BUF_SIZE] = { 0 };

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
  
  tcp_acceptargs_t accepterArguments = { 
    &m_ConnectedClients,
    &m_Socket4, &m_ClientAddress,
    &m_ClientAddressSize,
    m_Sockets, m_Clients
  };

  int threadCreation = pthread_create(&m_AcceptThread, NULL, ServerProperties::TcpAcceptLoop, &accepterArguments);
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
    int result = poll(m_Sockets, sizeof(m_Sockets)/sizeof(m_Sockets[0]), FPX_POLLTIMEOUT);

    if (result == -1) {
      printf("An error occured while polling sockets. %s\n", strerror(errno));
      continue;
    }
    if (result == 0) {
      //poll timed out
      continue;
    }

    short j = 0;
    for(short i = 1; i<FPX_MAX_CONNECTIONS+1 && j<result; i++) {

      bool handled = 0;
      if (m_Sockets[i].revents & POLLIN) {
        //ready to receive from device

        ssize_t bytesRead = read(m_Sockets[i].fd, readBuffer, FPX_BUF_SIZE);
        if (bytesRead > 0) {
          // printf("%s\n", readBuffer);
          if (!strncmp(readBuffer, FPX_INCOMING, strlen(FPX_INCOMING))) {
            char* cleanMsg = (char*)fpx_substr_replace(readBuffer, FPX_INCOMING, "");
            cleanMsg[strcspn(cleanMsg, "\r\n")] = 0;
            switch (*cleanMsg) {
              case '!':
                char temp[40];
                if (!strcmp(cleanMsg, "!online")) {
                  memset(temp, 0, sizeof(temp));
                  sprintf(writeBuffer, "\nHere is a list of connected clients (%d/%d):\n\n", m_ConnectedClients, FPX_MAX_CONNECTIONS);
                  short k=1;
                  for (ClientData& cnt : m_Clients) {
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
                    char recIDchars[4], message[FPX_BUF_SIZE], messageCpy[FPX_BUF_SIZE];
                    int recID;

                    memcpy(recIDchars, args, (strspn(args, "0123456789") > 4) ? 4 : strspn(args, "0123456789"));
                    recID = atoi(recIDchars);
                    args += strspn(args, "0123456789");
                    if (*args != ' ')
                      sprintf(writeBuffer, "Bad syntax: '!pm'. Requires '!pm [id] [message]'");
                    else {
                      args++;
                      sprintf(message, "Private from [%s (%d)]: ", m_Clients[i-1].Name, i);
                      memcpy(messageCpy, message, FPX_BUF_SIZE);
                      snprintf(message, FPX_BUF_SIZE, "%s%s\n", messageCpy, args);
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
            memset(readBuffer, 0, FPX_BUF_SIZE);
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
            snprintf(writeBuffer, FPX_BUF_SIZE, "%s", cleanMsg);
            write(m_Sockets[i].fd, writeBuffer, strlen(writeBuffer));
            memset(writeBuffer, 0, FPX_BUF_SIZE);
          }

          memset(readBuffer, 0, FPX_BUF_SIZE);
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

  m_IsListening = false;
  pthread_kill(m_AcceptThread, SIGTERM);
  pthread_join(m_AcceptThread, NULL);

  return;
}

void TcpServer::ListenSecure(const char* keypath, const char* certpath) {
  throw NotImplementedException("SSL/TLS is not yet implemented.");
}

void TcpServer::Close() {
  if (!m_IsListening) return;
  
  close(m_Socket4);

  pthread_kill(m_AcceptThread, SIGTERM);
  pthread_join(m_AcceptThread, NULL);
  m_AcceptThread = 0;

  m_Socket4 = 0;
  
  memset(m_Sockets, -1, sizeof(m_Sockets));
  memset(m_Clients, 0, sizeof(m_Clients));
  
  memset(&m_ClientAddress, 0, sizeof(m_ClientAddress));
  m_ClientAddressSize = 0;
  m_ConnectedClients = 0;

  m_IsListening = false;

  return;
}

void TcpServer::pvt_HandleDisconnect(pollfd& client, short& clientNumber) {
  m_ConnectedClients--;
  close(client.fd);
  printf("[%s (%d)] disconnected.\n", m_Clients[clientNumber-1].Name, clientNumber);
  client = { -1, -1, 0 };
  memset(&m_Clients[clientNumber-1], 0, sizeof(m_Clients[clientNumber-1]));
}

}

// HTTP server class
namespace fpx {

HttpServer::HttpServer(const char* ip, unsigned short port):
  TcpServer(ip, port), m_EndpointCount(0),
  m_RequestHandlers((http_threadpackage_t*)calloc(FPX_HTTP_THREADS, sizeof(http_threadpackage_t))),
  // m_WebsocketThreads((ServerProperties::threadpackage_t*)calloc(FPX_WEBSOCKETS_THREADS, sizeof(ServerProperties::threadpackage_t))),
  m_Endpoints((http_endpoint_t*)calloc(FPX_HTTP_ENDPOINTS, sizeof(http_endpoint_t))),
  m_DefaultHeaders(nullptr),
  Response101{  },
  Response404{  },
  Response405{  },
  Response413{  },
  Response426{  },
  Response505{  }
  {
    Response101.SetCode("101");
    Response101.SetStatus("Switching Protocols");
    Response101.SetHeaders("Connection: Upgrade\r\n");

    Response413.SetCode("413");
    Response413.SetStatus("Payload Too Large");

    Response404.SetCode("404");
    Response404.SetStatus("Not Found");
    Response404.SetHeaders("Content-Type: text/plain\r\n");
    Response404.SetBody("Endpoint not found.");

    Response405.SetCode("405");
    Response405.SetStatus("Method Not Allowed");
    Response405.SetHeaders("Content-Type: text/plain\r\n");

    Response426.SetCode("426");
    Response426.SetStatus("Upgrade Required");
    Response426.SetHeaders("Connection: Upgrade\r\nContent-Type: text/plain");

    Response505.SetCode("505");
    Response505.SetStatus("HTTP Version Not Supported");
    Response505.SetHeaders("Content-Type: text/plain\r\n");
    Response505.SetBody("HTTP version not supported. Try HTTP/1.1");
  }

const char* HttpServer::GetDefaultHeaders() {
  return m_DefaultHeaders;
}

void HttpServer::SetDefaultHeaders(const char* headers) {
  if (m_DefaultHeaders) {
    free(m_DefaultHeaders);
  }
  if(int len = fpx_getstringlength(headers)) {
    m_DefaultHeaders = (char*)malloc(len);
    memcpy(m_DefaultHeaders, headers, len);
  }
  return;
}

void HttpServer::CreateEndpoint(const char* uri,short methods, http_callback_t endpointCallback) {
  if (m_EndpointCount == FPX_HTTP_ENDPOINTS)
    throw Exception("Maximum endpoint count reached!");

  snprintf(m_Endpoints[m_EndpointCount].URI, 255, "%s", uri);
  m_Endpoints[m_EndpointCount].URI[255] = 0;
  m_Endpoints[m_EndpointCount].Callback = endpointCallback;
  m_Endpoints[m_EndpointCount].AllowedMethods = methods;
  m_EndpointCount++;
  return;
}

void HttpServer::Listen(ServerType mode) {
  int optvalTrue = 1;

  if (mode != ServerType::Http) throw NotImplementedException();

  if ((m_Socket4 = socket(AF_INET, SOCK_STREAM, 0)) < -1) {
    Close();
    throw NetException("Failed to create socket");
    return;
  }

  setsockopt(m_Socket4, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optvalTrue, sizeof(optvalTrue));

  if (bind(m_Socket4, (struct sockaddr*)&m_SocketAddress4, sizeof(m_SocketAddress4)) < 0) {
    Close();
    throw NetException("Failed to bind socket");
    return;
  }

  if (listen(m_Socket4, 16) < 0) {
    Close();
    throw NetException("Failed to listen on socket");
    return;
  }

  // create 2 threadpackages for WebSocket threads, stored in websocketThreads
  // m_WebsocketThreads = (threadpackage_t*)calloc(2, sizeof(threadpackage_t));

  // create 2 threads to handle incoming HTTP requests
  for(short i = 0; i<FPX_HTTP_THREADS; i++) {
    m_RequestHandlers[i].Endpoints = m_Endpoints;
    m_RequestHandlers[i].Caller = this;
    pthread_create(&m_RequestHandlers[i].Thread, NULL, ServerProperties::HttpProcessingThread, &m_RequestHandlers[i]);
  }


  m_IsListening = true;

  printf("HTTPserver listening on %s:%d\n", inet_ntoa(m_SocketAddress4.sin_addr), m_Port);
  sleep(1);

  while (m_IsListening) {
    int newClient = accept(m_Socket4, &m_ClientAddress, &m_ClientAddressSize);
    for (short i=0; i < FPX_HTTP_THREADS; i++) {
      if (!pthread_mutex_trylock(&m_RequestHandlers[i].TalkingStick)) {
        // printf("Sending client to thread %d (0-indexed)!\n", i);
        m_RequestHandlers[i].ClientFD = newClient;
        pthread_cond_signal(&m_RequestHandlers[i].Condition);
        pthread_mutex_unlock(&m_RequestHandlers[i].TalkingStick);
        break;
      }
    }
  }

}

void HttpServer::ListenSecure(ServerType mode, const char* keypath, const char* certpath) {
  throw NotImplementedException();
}

void HttpServer::Close() {
  if (!m_IsListening) return;
  
  close(m_Socket4);
  m_Socket4 = 0;

  for (short i=0; i < FPX_HTTP_THREADS; i++) {
    pthread_kill(m_RequestHandlers[i].Thread, SIGTERM);
    pthread_join(m_RequestHandlers[i].Thread, NULL);
  }

  memset(&m_ClientAddress, 0, sizeof(m_ClientAddress));
  m_ClientAddressSize = 0;

  m_IsListening = false;

  return;
}

bool HttpServer::http_response_t::SetCode(const char* code) {
  if(fpx_getstringlength(code) != 3) return false;
  memcpy(this->Code, code, 4);
  return true;
}

bool HttpServer::http_response_t::SetStatus(const char* status) {
  if(fpx_getstringlength(status) > 31) return false;
  memcpy(this->Status, status, fpx_getstringlength(status));
  return true;
}

bool HttpServer::http_response_t::SetHeaders(const char* headers) {
  // printf("Call set!\n");
  m_HeaderLen = fpx_getstringlength(headers);
  char* newAlloc;
  if (m_HeaderLen < 1) return false;
  // printf("Old: %x\n", this->Headers);
  if ((this->Headers = (this->Headers) ? (char*)realloc(this->Headers, m_HeaderLen+1) : (char*)malloc(m_HeaderLen+1)) == NULL) return false;
  // printf("this->Headers in SetHeaders: 0x%x\n", this->Headers);
  // printf("New: %x\n", newAlloc);
  memcpy(this->Headers, headers, m_HeaderLen);
  this->Headers[m_HeaderLen] = 0;
  return true;
}

bool HttpServer::http_response_t::SetBody(const char* body) {
  m_BodyLen = fpx_getstringlength(body);
  if (m_BodyLen < 1) return false;
  this->Body = (this->Body) ? (char*)realloc(this->Body, m_BodyLen) : (char*)malloc(m_BodyLen);
  // printf("this->Body in SetBody: 0x%x\n", this->Body);
  memcpy(this->Body, body, m_BodyLen);

  return true;
}

char* HttpServer::http_request_t::GetHeaderValue(const char* headerName) {
  char* headerLowercase = fpx_string_to_lower(headerName);
  char* allHeadersLowercase = fpx_string_to_lower(this->Headers);

  char* returnedString;
  
  int foundIndex;
  if ((foundIndex = fpx_substringindex(allHeadersLowercase, headerLowercase)) > -1) {
    char temp[256] = { 0 };
    int written = snprintf(temp, strcspn(this->Headers+foundIndex+fpx_getstringlength(headerName)+1, "\r\n"), "%s\r\n", this->Headers+foundIndex+fpx_getstringlength(headerName)+2);
    returnedString = (char*)malloc(written+1);
    // printf("returnedString in GetHeaderValue: 0x%x\n", returnedString);
    memcpy(returnedString, temp, written);
    returnedString[written] = 0;
  } else {
    return (char*)"Header not found";
  }

  free(headerLowercase);
  free(allHeadersLowercase);

  return returnedString;
}

int HttpServer::http_response_t::GetHeaderLength() { return m_HeaderLen; }

int HttpServer::http_response_t::GetBodyLength() { return m_BodyLen; }

int HttpServer::http_response_t::AddHeader(const char* newHeader) {
  // printf("Call add!\n");
  int addedLen = fpx_getstringlength(newHeader);
  char* newAlloc;
  if (!this->Headers) {
    this->Headers = (char*)malloc(fpx_getstringlength(newHeader)+2);
    // printf("this->Headers in AddHeader: 0x%x\n", this->Headers);
  } else
  if (fpx_substringindex(this->Headers, newHeader) < 0) {
    // printf("Old: %x\n", this->Headers);
    if ((this->Headers = (char*)realloc(this->Headers, m_HeaderLen + addedLen + 2)) == NULL) return -1;
    // printf("New: %x\n", newAlloc);
  } else return -1;
  memcpy(this->Headers + m_HeaderLen, newHeader, addedLen);
  memcpy(this->Headers + m_HeaderLen + addedLen, "\r\n", 2);
  m_HeaderLen += addedLen+2;
  return (addedLen + 2);
}


HttpServer::HttpMethod HttpServer::ParseMethod(const char* method) {
  if (!strcmp(method, "GET")) return HttpServer::GET;
  if (!strcmp(method, "HEAD")) return HttpServer::HEAD;
  if (!strcmp(method, "POST")) return HttpServer::POST;
  if (!strcmp(method, "PUT")) return HttpServer::PUT;
  if (!strcmp(method, "DELETE")) return HttpServer::DELETE;
  if (!strcmp(method, "CONNECT")) return HttpServer::CONNECT;
  if (!strcmp(method, "OPTIONS")) return HttpServer::OPTIONS;
  if (!strcmp(method, "TRACE")) return HttpServer::TRACE;
  if (!strcmp(method, "PATCH")) return HttpServer::PATCH;
  return HttpServer::NONE;
}

}

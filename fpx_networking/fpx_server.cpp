////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

#include "fpx_server.h"

// loose functions declared here
namespace fpx::ServerProperties {

void* TcpAcceptLoop(void* arguments) {
  if (!arguments) return nullptr;
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
  if (!threadpack) return nullptr;
  // printf("An HTTP-processing thread has started!\n");
  HttpServer::http_threadpackage_t* package = (HttpServer::http_threadpackage_t*)threadpack;

  char readBuffer[FPX_HTTP_READ_BUF+1];
  char writeBuffer[FPX_HTTP_WRITE_BUF];

  char* voidpointer = nullptr;

  while (1) {
    pthread_mutex_lock(&package->TalkingStick);
    pthread_cond_wait(&package->Condition, &package->TalkingStick);
    
    char method[8];
    long totalWritten;
    char clientUpgraded = 0;
    bool keepalive = true, done = false, wsUpgrade = false, wsFail = false;

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

      if (amountRead == FPX_HTTP_READ_BUF+1) {
        response.CopyFrom(&package->Caller->Response413);
        keepalive = false;
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

      // vvv This line was the cause of so much anguish </3
      // memset(&request.Headers[headerLength-2], 0, 2);

      int bodyRead;
      sscanf(&readBuffer[firstLineRead+fpx_getstringlength(request.Headers)+2], "%2799s%n", request.Body, &bodyRead);
      request.Method = HttpServer::ParseMethod(method);

      if (!request.GetHeaderValue("Host", &voidpointer, true, false)) {
        response.CopyFrom(&package->Caller->Response400);
        goto buildFromPreset;
      }

      printf("Serving '%s' to client. ", request.URI); fflush(stdout);

      if ((strcmp(request.Version, "HTTP/1.1"))) {
        response.CopyFrom(&package->Caller->Response505);
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
        {
          char* connHeader = nullptr;
          request.GetHeaderValue("Connection", &connHeader);

          char* upHeader = nullptr;
          request.GetHeaderValue("Upgrade", &upHeader);

          if(upHeader && ((connHeader) ? fpx_substringindex(connHeader, "upgrade") > -1 : 0)) {
            // we be upgrading :nydance:
            if(!strcmp(upHeader, "websocket")) {// && keyHeader && (!strcmp(verHeader, "13")) &&
              // package->Caller->Mode != HttpServer::ServerType::HttpOnly
              // request wants to upgrade to websocket! poggies!
              char* keyHeader = nullptr;
              request.GetHeaderValue("Sec-WebSocket-Key", &keyHeader, false);

              char* verHeader = nullptr;
              request.GetHeaderValue("Sec-WebSocket-Version", &verHeader);

              char* protHeader = nullptr;
              request.GetHeaderValue("Sec-WebSocket-Protocol", &protHeader);
              
              if (keyHeader && ((verHeader) ? !strcmp(verHeader, "13") : false) && package->Caller->Mode != HttpServer::ServerType::HttpOnly) {
                response.CopyFrom(&package->Caller->Response101);
                char* accept_part_1 = fpx_substr_replace(keyHeader, "==", "==258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
                char* accept_part_2 = (char*)calloc(21, 1);
                fpx_sha1_digest(accept_part_1, fpx_getstringlength(accept_part_1), accept_part_2, 0);
                char* accept_part_3 = fpx_base64_encode(accept_part_2, 20);
                char* accept_part_4 = (char*)calloc(51, 1);
                strcpy(accept_part_4, "Sec-WebSocket-Accept: ");
                strncat(accept_part_4, accept_part_3, 28);
                response.AddHeader(accept_part_4);
                wsUpgrade = true;
                free(accept_part_1);
                free(accept_part_2);
                free(accept_part_3);
                free(accept_part_4);
              } else if (package->Caller->Mode == HttpServer::ServerType::HttpOnly) {
                // 501 - Not Implemented | https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/501
                // because server does not support WebSocket
                response.CopyFrom(&package->Caller->Response501);
                wsFail = true;
              } else {
                // 400 - Bad Request | https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/400
                // because missing key or wrong version (should be 13)
                response.CopyFrom(&package->Caller->Response400);
                response.SetBody("Could not upgrade request because of incorrect request header composition.\nRefer to [RFC-6455] for information about the required request headers and their validity.");
                wsFail = true;
              }
              if (keyHeader) {
                free(keyHeader);
                keyHeader = nullptr;
              }
              if (verHeader) {
                free(verHeader);
                verHeader = nullptr;
              }
              if (protHeader) {
                free(protHeader);
                protHeader = nullptr;
              }
              done = true;
            }
          }

          if (connHeader) {
            free(connHeader);
            connHeader = nullptr;
          }
          if (upHeader) {
            free(upHeader);
            upHeader = nullptr;
          }
        }

        if (endpointPtr && (request.Method & endpointPtr->AllowedMethods)) {
          if (!done || (package->Caller->GetOptions() & HttpServer::HttpServerOption::ManualWebSocket))
            endpointPtr->Callback(&request, &response);
        } else if (endpointPtr) {
          response.CopyFrom(&package->Caller->Response405);
        } else if (package->Caller->GetOptions() & HttpServer::HttpServerOption::ManualWebSocket) {
          // 404 not found
          response.CopyFrom(&package->Caller->Response404);
        }
      }
      
      buildFromPreset:

      sprintf(response.Version, "HTTP/1.1");

      if (wsUpgrade) {
        response.AddHeader("Upgrade: websocket");
        response.AddHeader("Connection: Upgrade");
      }

      char* connectionHeader = nullptr;

      if (request.GetHeaderValue("Connection", &connectionHeader) && response.Headers) {
        char* respHeadersLower = fpx_string_to_lower(response.Headers, 1);
        if (fpx_substringindex(respHeadersLower, "connection: ") == -1) {
          // printf("Checking keepalive\n");
          if (!strcmp(connectionHeader, "close")) {
            // printf("Found 'close'\n");
            keepalive = false;
          }
          else {
            // printf("Found 'keep-alive'\n");
            response.AddHeader("Connection: keep-alive");
            response.AddHeader("Keep-Alive: timeout=7");
          }
        }
        free(respHeadersLower);
      }

      free(connectionHeader); 

      short amountWritten;

      if (!keepalive) {
        response.AddHeader("Connection: close");
      }

      if (response.GetBodyLength()) {
        char tempHeaderBuf[256];
        sprintf(tempHeaderBuf, "Content-Length: %d", response.GetBodyLength());
        response.AddHeader(tempHeaderBuf);
      }

      amountWritten = snprintf(writeBuffer, FPX_HTTP_WRITE_BUF, "%s %s %s\r\n",
        response.Version, response.Code, response.Status
      );
      memcpy(writeBuffer+amountWritten, package->Caller->GetDefaultHeaders(), fpx_getstringlength(package->Caller->GetDefaultHeaders()));
      amountWritten+=fpx_getstringlength(package->Caller->GetDefaultHeaders());
      if (response.Headers) {
        memcpy(writeBuffer+amountWritten, response.Headers, fpx_getstringlength(response.Headers));
        amountWritten+=fpx_getstringlength(response.Headers);
      }
      memcpy(writeBuffer+amountWritten, "\r\n", 2);
      amountWritten+=2;
      if (response.Body) {
        memcpy(writeBuffer+amountWritten, response.Body, fpx_getstringlength(response.Body));
        amountWritten+=fpx_getstringlength(response.Body);
      }

      if (response.Headers) free(response.Headers);
      if (response.Body) free(response.Body);
      memset(&request, 0, sizeof(request));
      memset(&response, 0, sizeof(response));
      totalWritten += amountWritten;

      endpointPtr = nullptr;

      if (wsUpgrade) {
        short lowestCount = FPX_WS_MAX_CLIENTS;
        short lowestIndex = 1;
        for (short i=0; i < FPX_HTTP_THREADS; i++) {
          if (package->Caller->WebsocketThreads[i].ClientCount < lowestCount) {
            lowestCount = package->Caller->WebsocketThreads[i].ClientCount;
            lowestIndex = i;
          }
        }
        short* ccPTR = &package->Caller->WebsocketThreads[lowestIndex].ClientCount;
        uint8_t temp[128] = { 0b10000001, 0b00000011, 'a', 'b', 'c' };
        pthread_mutex_lock(&package->Caller->WebsocketThreads[lowestIndex].TalkingStick) && !(package->Caller->GetOptions() & HttpServer::HttpServerOption::ManualWebSocket && endpointPtr);
        printf("(Upgrading to WebSocket)\n");
        send(package->ClientFD, writeBuffer, totalWritten, 0);
        send(package->ClientFD, temp, 5, 0);
        package->Caller->WebsocketThreads[lowestIndex].PollFDs[package->Caller->WebsocketThreads[lowestIndex].ClientCount] = client;
        package->Caller->WebsocketThreads[lowestIndex].Clients[*ccPTR].LastActiveSeconds = time(NULL);
        (*ccPTR)++;
        pthread_cond_signal(&package->Caller->WebsocketThreads[lowestIndex].Condition);
        pthread_mutex_unlock(&package->Caller->WebsocketThreads[lowestIndex].TalkingStick);
        
        keepalive = false;
      } else {
        send(package->ClientFD, writeBuffer, totalWritten, 0);
        printf("\n");
      }
      
      memset(readBuffer, 0, sizeof(readBuffer));
      memset(writeBuffer, 0, sizeof(writeBuffer));
      totalWritten = 0;

      if (!keepalive)
        if (!(wsUpgrade)) close(package->ClientFD);
        break;
    }
    package->ClientFD = 0;
    pthread_mutex_unlock(&package->TalkingStick);
  }

  return NULL;
}

void* WebSocketThread(void* threadpack) {
  if (!threadpack) return nullptr;
  HttpServer::websocket_threadpackage_t* package = (HttpServer::websocket_threadpackage_t*)threadpack;

  uint16_t metadata;
  uint16_t bigendianChecker = 1;
  bool bigendian = (*(char*)&bigendianChecker) ? false : true;
  uint8_t writeBuffer[FPX_BUF_SIZE];
  bool locked = false;

  while (1) {
    // printf("locking\n");
    pthread_mutex_lock(&package->TalkingStick);
    if (!package->ClientCount)
      pthread_cond_wait(&package->Condition, &package->TalkingStick);

    short clientsReady = poll(package->PollFDs, package->ClientCount, 1000);
    // printf("Clients ready on thread %d: %d\n", package->Thread, clientsReady);

    int j=0;
    for (short i=0; i < package->ClientCount && j < clientsReady; i++) {
      int handled = 0;
      bool disconnect = false;
      // handle polled clients here ^_^
      if(package->PollFDs[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        // bad client connection OR client hung up
        package->HandleDisconnect(i);
        handled++;
        j++;
        continue;
      }
      if(package->PollFDs[i].revents & POLLIN) {
        // receive client data
        ssize_t bytesRead = read(package->PollFDs[i].fd, (char*)&metadata, 2);
        if (!bigendian) fpx_endian_swap(&metadata, 2);
        switch (bytesRead) {
          case 0:
            // EOF (client closed connection)
            handled++;
            disconnect = true;
            // printf("WS client DC'ed\n");
            break;
          case -1:
            // error happened
            break;
          default:
            // READREADREAD!!!
            // https://datatracker.ietf.org/doc/html/rfc6455#section-5.2

            // (!(MASK)              || (RSV1,2,3)         )
            if (!(metadata & 0x0080) || (metadata & 0x7000)) {
              handled++;
              disconnect = true;
              break;
            }
            
            uint8_t fin, opcode;
            uint64_t len;
            uint32_t mask_key;
            bool finishedReading = false;

            fin = metadata >> 15;
            opcode = (metadata >> 8) & 0xf;
            {
              uint8_t firstLen = metadata & 0x007f;
              if (firstLen > 125) {
                if (!read(package->PollFDs[i].fd, &len, (firstLen == 126) ? 2 : 8)) {
                  handled++;
                  break;
                }
              } else len = firstLen;
              if (firstLen == 127) {
                len &= 0x7fffffffffffffff;
              }
            }

            if (!read(package->PollFDs[i].fd, &mask_key, 4)) {
              handled++;
              break;
            }

            if (len > FPX_WS_BUFFER) {
              char closeBuf [64] = { 0 };
              snprintf(closeBuf, 63, "disconnect_body_over_%hubytes", FPX_WS_BUFFER);
              package->SendClose(i, 1009, (uint8_t*)closeBuf);
            }

            if (opcode & 0x8) {
              // control frame
              if (!fin || len > 125) {
                if (len) {
                  package->Clients[i].ReadBufferPTR = (!package->Clients[i].ReadBufferPTR) ? (uint8_t*)malloc(len) : (uint8_t*)realloc(package->Clients[i].ReadBufferPTR, len);
                  read(package->PollFDs[i].fd, package->Clients[i].ReadBufferPTR, len);
                  free(package->Clients[i].ReadBufferPTR);
                  package->Clients[i].ReadBufferPTR = nullptr;
                }
                handled++;
                break;
              }

              ssize_t ctlBuffer_read = read(package->PollFDs[i].fd, package->Clients[i].ControlReadBuffer, len);
              if (!ctlBuffer_read) {
                handled++;
                break;
              }
              for (int j=0; j < len; j++) {
                package->Clients[i].ControlReadBuffer[j] ^= ((uint8_t*)&mask_key)[j % 4];
              }
              switch (opcode) {
                case 0x8:
                  // connection close
                  if (package->Clients[i].Flags & FPX_WS_RECV_CLOSE) {
                    handled++;
                    break;
                  }
                  if (package->Clients[i].PendingClose) {
                    // conn-close confirmed by client 👍
                  } else {
                    package->SendClose(i, *((uint16_t*)package->Clients[i].ControlReadBuffer));
                  }

                  package->Clients[i].Flags |= FPX_WS_RECV_CLOSE;
                  handled++;
                  disconnect = true;
                  break;

                case 0x9:
                  // ping
                  if (len) memcpy(writeBuffer + snprintf((char*)writeBuffer, FPX_BUF_SIZE, "\x8a%u", len), package->Clients[i].ControlReadBuffer, len);
                  send(package->PollFDs[i].fd, writeBuffer, 2+len, 0);
                  handled++;
                  break;

                case 0xa:
                  // pong
                  
                  break;

              }
              memset(package->Clients[i].ControlReadBuffer, 0, sizeof(package->Clients[i].ControlReadBuffer));
            } else {
              // data frame
              
              size_t oldsize = package->Clients[i].ReadBufSize;
              if (package->Clients[i].ReadBufferPTR && package->Clients[i].Fragmented) {
                package->Clients[i].ReadBufferPTR = (uint8_t*)realloc(package->Clients[i].ReadBufferPTR, (package->Clients[i].ReadBufSize += len+1) & FPX_WS_BUFFER);
              } else
              if (package->Clients[i].ReadBufferPTR) {
                package->Clients[i].ReadBufferPTR = (uint8_t*)realloc(package->Clients[i].ReadBufferPTR, (package->Clients[i].ReadBufSize = len+1) & FPX_WS_BUFFER);
              } else package->Clients[i].ReadBufferPTR = (uint8_t*)malloc((package->Clients[i].ReadBufSize = len+1) & FPX_WS_BUFFER);
              ssize_t frameBodyLengthRead = read(package->PollFDs[i].fd, package->Clients[i].ReadBufferPTR + oldsize, len);
              package->Clients[i].ReadBufferPTR[len] = 0;

              if ((package->Clients[i].Fragmented && opcode) || !(package->Clients[i].Fragmented || opcode)) {
                // flush this frame, because it's:
                // - not a continuation frame where expected
                // - a continuation frame where unexpected
                uint8_t tmpBuf[len];
                read(package->PollFDs[i].fd, tmpBuf, len);
                handled++;
                break;
              }


              // unmask client payload
              for (int j=0; j < len; j++) {
                package->Clients[i].ReadBufferPTR[j] ^= ((uint8_t*)&mask_key)[j % 4];
              }
              
              // strip readbuffer of \r or \r\n
              if (package->Clients[i].ReadBufferPTR[frameBodyLengthRead-1] == '\n') {
                if (package->Clients[i].ReadBufferPTR[frameBodyLengthRead-2] == '\r') {
                  package->Clients[i].ReadBufferPTR[frameBodyLengthRead-2] = 0;
                }
                package->Clients[i].ReadBufferPTR[frameBodyLengthRead-1] = 0;
              }
              
              // switch (opcode) {
              //   case 0x0:
              //     // continuation frame
              //     break;
              //   case 0x1:
              //     // text frame
              //     // printf("msg but not callback: %s\n", package->Clients[i].ReadBufferPTR);
              //     break;
              //   case 0x2:
              //     // binary frame
              //     break;
              // }
              if (package->Callback) package->Callback(&package->Clients[i], metadata, len, mask_key, package->Clients[i].ReadBufferPTR);
              // printf("\n");
              if (fin) {
                package->Clients[i].Fragmented = false;
                free(package->Clients[i].ReadBufferPTR);
                package->Clients[i].ReadBufferPTR = nullptr;
                package->Clients[i].ReadBufSize = 0;
              }
            }
            break;
        }
        if (handled) {
          j++;
          if (!package->Clients[i].Fragmented && package->Clients[i].ReadBufferPTR) {
            free(package->Clients[i].ReadBufferPTR);
            package->Clients[i].ReadBufferPTR = nullptr;
          }
          time(&package->Clients[i].LastActiveSeconds);
        }
        if (disconnect) package->HandleDisconnect(i);
      }
    }

    for(int i=0; i < package->ClientCount; i++) {
      // printf("Time now: %u | Client last active: %u | Difference: %u\n", time(NULL)/60, package->Clients[i].LastActiveSeconds/60, time(NULL)/60 - package->Clients[i].LastActiveSeconds/60);
      if((package->Caller->GetWebSocketTimeout()) && (time(NULL)/60 - package->Clients[i].LastActiveSeconds/60) > package->Caller->GetWebSocketTimeout()) {
        uint8_t message[123];
        snprintf((char*)message, 123, "disconnect_idle_%hu_minutes", package->Caller->GetWebSocketTimeout());
        package->SendClose(i, 1000, message);
        package->Clients[i].PendingClose = true;
      }
    }
    // printf("unlocking\n");
    pthread_mutex_unlock(&package->TalkingStick);
    usleep(5000);
  }

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
                    }
                    free((char*)argsCpy);
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
            if (*clientName != 0) {
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
  RequestHandlers((http_threadpackage_t*)calloc(FPX_HTTP_THREADS, sizeof(http_threadpackage_t))),
  WebsocketThreads((websocket_threadpackage_t*)calloc(FPX_WS_THREADS, sizeof(websocket_threadpackage_t))),
  m_Endpoints((http_endpoint_t*)calloc(FPX_HTTP_ENDPOINTS, sizeof(http_endpoint_t))),
  m_DefaultHeaders(nullptr), m_Options(0), m_WebSocketTimeout(0),
  Response101{  },
  Response400{  },
  Response404{  },
  Response405{  },
  Response413{  },
  Response426{  },
  Response505{  }
  {
    SetDefaultHeaders(
      "Server: fpxHTTP ("
      FPX_HTTPSERVER_VERSION
      ")\r\n"
    );

    Response101.SetCode("101");
    Response101.SetStatus("Switching Protocols");
    Response101.SetHeaders("Connection: Upgrade\r\n");

    Response400.SetCode("400");
    Response400.SetStatus("Bad Request");
    Response400.SetBody("Bad Request");

    Response413.SetCode("413");
    Response413.SetStatus("Payload Too Large");
    Response413.SetBody("Payload Too Large");

    Response404.SetCode("404");
    Response404.SetStatus("Not Found");
    Response404.SetHeaders("Content-Type: text/plain\r\n");
    Response404.SetBody("404 Not Found");

    Response405.SetCode("405");
    Response405.SetStatus("Method Not Allowed");
    Response405.SetHeaders("Content-Type: text/plain\r\n");

    Response426.SetCode("426");
    Response426.SetStatus("Upgrade Required");
    Response426.SetHeaders("Connection: Upgrade\r\nContent-Type: text/plain");
    Response426.SetBody("Upgrade Required");

    Response501.SetCode("501");
    Response501.SetStatus("Not Implemented");
    Response501.SetHeaders("Content-Type: text/plain\r\n");
    Response501.SetBody("Not Implemented");

    Response505.SetCode("505");
    Response505.SetStatus("HTTP Version Not Supported");
    Response505.SetHeaders("Content-Type: text/plain\r\n");
    Response505.SetBody("HTTP Version Not Supported. Try HTTP/1.1");
  }

HttpServer::~HttpServer() {
  free(RequestHandlers);
  free(WebsocketThreads);
  free(m_Endpoints);
  if (m_DefaultHeaders) free(m_DefaultHeaders);
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

void HttpServer::Listen(ServerType mode, ws_callback_t websocketCallback) {
  Mode = mode;
  
  int optvalTrue = 1;

  if (mode == ServerType::WebSockets) throw NotImplementedException();

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

  // create 2 threads to handle incoming HTTP requests
  for(short i = 0; i<FPX_HTTP_THREADS; i++) {
    RequestHandlers[i].Endpoints = m_Endpoints;
    RequestHandlers[i].Caller = this;
    pthread_create(&RequestHandlers[i].Thread, NULL, ServerProperties::HttpProcessingThread, &RequestHandlers[i]);
  }

  // create 2 threadpackages for WebSocket threads
  for(short i = 0; i<FPX_HTTP_THREADS; i++) {
    WebsocketThreads[i].Caller = this;
    WebsocketThreads[i].Callback = websocketCallback;
    pthread_create(&WebsocketThreads[i].Thread, NULL, ServerProperties::WebSocketThread, &WebsocketThreads[i]);
  }

  m_IsListening = true;

  printf("HTTPserver listening on %s:%d\n", inet_ntoa(m_SocketAddress4.sin_addr), m_Port);
  // usleep(500000);

  while (m_IsListening) {
    int newClient = accept(m_Socket4, &m_ClientAddress, &m_ClientAddressSize);
    while (newClient) {
      for (short i=0; i < FPX_HTTP_THREADS; i++) {
        if (!pthread_mutex_trylock(&RequestHandlers[i].TalkingStick)) {
          // printf("Sending client to thread %d (0-indexed)!\n", i);
          RequestHandlers[i].ClientFD = newClient;
          pthread_cond_signal(&RequestHandlers[i].Condition);
          pthread_mutex_unlock(&RequestHandlers[i].TalkingStick);
          newClient = 0;
          break;
        }
      }
    }
  }

}

void HttpServer::ListenSecure(ServerType mode, const char* keypath, const char* certpath, ws_callback_t websocketCallback) {
  throw NotImplementedException();
}

void HttpServer::Close() {
  if (!m_IsListening) return;
  
  close(m_Socket4);
  m_Socket4 = 0;

  for (short i=0; i < FPX_HTTP_THREADS; i++) {
    pthread_kill(RequestHandlers[i].Thread, SIGTERM);
    pthread_join(RequestHandlers[i].Thread, NULL);
  }

  memset(&m_ClientAddress, 0, sizeof(m_ClientAddress));
  m_ClientAddressSize = 0;

  m_IsListening = false;

  return;
}

void HttpServer::SetWebSocketTimeout(uint16_t minutes) {
  m_WebSocketTimeout = minutes;
}

uint16_t HttpServer::GetWebSocketTimeout() {
  return m_WebSocketTimeout;
}

void HttpServer::http_response_t::CopyFrom(struct HttpServer::Http_Response* other) {
  int otherHLen, otherBLen;
  memcpy(Version, other->Version, sizeof(Version));
  memcpy(Code, other->Code, sizeof(Code));
  memcpy(Status, other->Status, sizeof(Status));
  otherHLen = other->GetHeaderLength();
  otherBLen = other->GetBodyLength();
  if (other->Headers && otherHLen) {
    if (Headers) Headers = (char*)realloc(Headers, otherHLen);
    else  Headers = (char*)malloc(otherHLen);
    memcpy(Headers, other->Headers, otherHLen);
  }
  if (other->Body && otherBLen) {
    if (Body) Body = (char*)realloc(Body, otherBLen);
    else  Body = (char*)malloc(otherBLen);
    memcpy(Body, other->Body, otherBLen);
  }
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

bool HttpServer::http_request_t::GetHeaderValue(const char* headerName, char** resultStore, bool lowercase, bool storeValue) {
  char* header = (char*)malloc(fpx_getstringlength(headerName)+3);
  strcpy(header, headerName);
  strcat(header, ": ");
  fpx_string_to_lower(header, 0);
  char* lowercaseHeaders = fpx_string_to_lower(this->Headers, 1);

  if (!storeValue) {
    bool retval = fpx_substringindex(lowercaseHeaders, header) > -1;
    free(header);
    free(lowercaseHeaders);
    return retval;
  }

  char* returnedString;
  
  int foundIndex;
  if (((foundIndex = fpx_substringindex(lowercaseHeaders, header)) > -1) && storeValue) {
    char temp[256] = { 0 };
    int written = strcspn(this->Headers+foundIndex+fpx_getstringlength(headerName)+1, "\r\n");
    snprintf(temp, written, "%s", this->Headers+foundIndex+fpx_getstringlength(headerName)+2);
    returnedString = (char*)malloc(written+1);
    memcpy(returnedString, temp, written);
    returnedString[written] = 0;
  } else if (storeValue) {
    returnedString = nullptr;
  }
  
  free(header);
  free(lowercaseHeaders);
  if (lowercase && returnedString) fpx_string_to_lower(returnedString, 0);
  *resultStore = returnedString;
  return foundIndex > -1;
}

int HttpServer::http_response_t::GetHeaderLength() { return m_HeaderLen; }

int HttpServer::http_response_t::GetBodyLength() { return m_BodyLen; }

int HttpServer::http_response_t::AddHeader(const char* newHeader, bool freeOld) {
  // printf("Call add!\n");
  int addedLen = fpx_getstringlength(newHeader);
  char* newAlloc;
  if ((!this->Headers) || freeOld) {
    if (this->Headers) {
      int oldheadLen = fpx_getstringlength(this->Headers);
      char oldHead[oldheadLen+1];
      strcpy(oldHead, this->Headers);
      char* oldHeadersPtr = this->Headers;
      this->Headers = (char*)malloc(fpx_getstringlength(newHeader)+2);
      free(oldHeadersPtr);
      strcpy(this->Headers, oldHead);
    } else this->Headers = (char*)malloc(fpx_getstringlength(newHeader)+2);
    // printf("this->Headers in AddHeader: 0x%x\n", this->Headers);
  }
  if (fpx_substringindex(this->Headers, newHeader) < 0) {
    // printf("Old: %x\n", this->Headers);
    if ((this->Headers = (char*)realloc(this->Headers, m_HeaderLen + addedLen + 3)) == NULL) return -1;
    // printf("New: %x\n", newAlloc);
  } else return -1;
  memcpy(this->Headers + m_HeaderLen, newHeader, addedLen);
  memcpy(this->Headers + m_HeaderLen + addedLen, "\r\n", 3);
  m_HeaderLen += addedLen+2;
  return (addedLen + 2);
}

void HttpServer::websocket_frame_t::SetBit(uint8_t bit, bool value) {
  if (value) m_MetaByte |= bit;
  else m_MetaByte &= ~bit;
}

void HttpServer::websocket_frame_t::SetOpcode(uint8_t opcode) {
  m_MetaByte |= (opcode & 0x0f);
}

bool HttpServer::websocket_frame_t::SetPayload(char* payload, uint16_t len) {
  if (!payload) return false;

  if (m_PayloadLen >= len && m_Payload) memset(&m_Payload[len], 0, m_PayloadLen - len);
  else if (m_Payload) m_Payload = (char*)realloc(m_Payload, len);
  else m_Payload = (char*)malloc(len);

  m_PayloadLen = len;

  memcpy(m_Payload, payload, len);

  return true;
}

void HttpServer::websocket_threadpackage_t::HandleDisconnect(int clientIndex) {
  bool moveMode = false, done = false;
  short index = 0;
  
  if (PollFDs[clientIndex].fd == 0) {
    throw IndexOutOfRangeException("There is no WebSocket client with that index");
  }

  if (Clients[clientIndex].ReadBufferPTR) {
    free(Clients[clientIndex].ReadBufferPTR);
    memset(&Clients[clientIndex], 0, sizeof(websocket_client_t));
  }

  close(PollFDs[clientIndex].fd);

  for (int i=clientIndex; i < FPX_WS_MAX_CLIENTS; i++) {
    if (i+1 < FPX_WS_MAX_CLIENTS) {
      PollFDs[i] = PollFDs[i+1];
      if (PollFDs[i+1].fd == 0) break;
    } else {
      memset(&PollFDs[i], 0, sizeof(pollfd));
      memset(&Clients[i], 0, sizeof(websocket_client_t));
      break;
    }
  }
  Clients[clientIndex].PendingClose = false;
  ClientCount -= 1;

  // vvv OLD CODE vvv
  // for (pollfd& client : PollFDs) {
  //   switch (moveMode) {
  //     case true:
  //       if (client.fd == 0) {
  //         done = true;
  //         break;
  //       }
  //       PollFDs[index-1] = client;
  //       memset(&client, 0, sizeof(client));
  //       break;
  //     case false:
  //       if (client.fd == PollFDs[clientIndex].fd) {
  //         close(client.fd);
  //         ClientCount -= 1;
  //         moveMode = true;
  //         memset(&client, 0, sizeof(client));
  //       }
  //       break;
  //   }
  //   if (done) break;
  //   index++;
  // }
}

void HttpServer::websocket_threadpackage_t::SendFrame(int index, websocket_frame_t*) {
  throw NotImplementedException();
}

void HttpServer::websocket_threadpackage_t::SendClose(int index, uint16_t status, uint8_t* message) {
  status = htons(status);
  uint8_t buflen = 4;
  uint8_t stringlength = (message) ? (uint8_t)fpx_getstringlength((char*)message) : 0;
  stringlength = (stringlength > 123) ? 123 : stringlength;
  if (message) buflen += stringlength;
  uint8_t writebuf[buflen];
  writebuf[0] = 0x88;
  writebuf[1] = buflen-2;
  ((uint16_t*)writebuf)[1] = status;
  if (message) {
    memcpy(&writebuf[4], (uint8_t*)message, stringlength);
  }
  send(PollFDs[index].fd, writebuf, buflen, 0);
  Clients[index].PendingClose = true;
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

void HttpServer::SetOption(HttpServerOption option, bool value) {
  if (value)
    m_Options |= option;
  else
    m_Options &= (0xff - option);
}

uint8_t HttpServer::GetOptions() {
  return m_Options;
}

}

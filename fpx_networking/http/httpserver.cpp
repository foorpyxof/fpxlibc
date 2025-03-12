////////////////////////////////////////////////////////////////
//  "httpserver.cpp"                                          //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

static int count = 0;

#include "httpserver.h"

#include "../../fpx_cpp-utils/exceptions.h"
extern "C" {
#include "../../fpx_c-utils/crypto.h"
#include "../../fpx_c-utils/endian.h"
#include "../../fpx_string/string.h"
}

#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FPX_HTTP_ENDPOINTS 64

#define FPX_HTTP_READ_BUF 4096
#define FPX_HTTP_WRITE_BUF 8192

#define FPX_HTTP_KEEPALIVE 7

#define FPX_HTTPSERVER_VERSION "alpha:mar-2025"

#define FPX_WS_READBUF 0xffff
#define FPX_WS_WRITEBUF 2048

#define WS_FIN 0b10000000
#define WS_RSV1 0b01000000
#define WS_RSV2 0b00100000
#define WS_RSV3 0b00010000

#define FPX_WS_SEND_CLOSE 0x1
#define FPX_WS_RECV_CLOSE 0x2

namespace fpx {

namespace ServerProperties {

/**
 * Takes a pointer to an fpx::threadpackage_t object.
 */
static void* HttpProcessingThread(void* threadpack) {
  if (!threadpack)
    return nullptr;

  HttpServer::http_threadpackage_t* package = (HttpServer::http_threadpackage_t*)threadpack;

  char writeBuffer[FPX_HTTP_WRITE_BUF];

  while (1) {
    char* voidpointer = nullptr;

    pthread_mutex_lock(&package->TalkingStick);
    if (!package->ClientCount)
      pthread_cond_wait(&package->Condition, &package->TalkingStick);

    // printf("pog we polling\n");
    short clientsReady = poll(package->PollFDs, package->ClientCount, 1000);

    short j = 0;
    // printf("%d ready\n", clientsReady);

    for (short i = 0; i < package->ClientCount && j < clientsReady; i++) {
      HttpServer::http_client_t* cli = &package->Clients[i];
      pollfd* pfd = &package->PollFDs[i];
      int handled = 0;

      if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
        // bad socket or client hung up
        package->HandleDisconnect(i);
        j++;
        continue;
      }

      if (pfd->revents & POLLIN) {
        if (!cli->ReadBufferPTR) {
          cli->ReadBufferPTR = (char*)malloc(FPX_HTTP_READ_BUF);
        }
        ssize_t amountRead = recv(pfd->fd, cli->ReadBufferPTR, FPX_HTTP_READ_BUF + 1, 0);
        if (!amountRead) {
          j++;
          continue;
        }

        if (amountRead == FPX_HTTP_READ_BUF + 1) {
          cli->Response.CopyFrom(&package->Caller->Response413);
          // goto buildResponseFromPreset; // (?)
        }

        int firstLineRead = 0, headerLen = 0;
        char method[8] = { 0 };
        sscanf(cli->ReadBufferPTR,
          "%s %255s %15s\r\n%n",
          method,
          cli->Request.URI,
          cli->Request.Version,
          &firstLineRead);
        cli->ReadBufSize += firstLineRead;
        for (int i = cli->ReadBufSize; i < 1024 + firstLineRead;) {
          int newRead;
          char tempBuf[256];
          sscanf(&cli->ReadBufferPTR[i], "%255[^\r\n]\r\n%n", tempBuf, &newRead);
          if (newRead > 0 && !strncmp(&cli->ReadBufferPTR[i + newRead - 2], "\r\n", 2)) {
            cli->ReadBufSize += newRead;
            if (1024 - strlen(cli->Request.Headers) < newRead)
              break;  // header section too large
            memcpy(&cli->Request.Headers[i - firstLineRead], &cli->ReadBufferPTR[i], newRead);
            memset(tempBuf, 0, sizeof(tempBuf));
            i += newRead;
            headerLen += newRead;
          } else
            break;
        }

        {
          char* cl_header = nullptr;
          if (cli->Request.GetHeaderValue("Content-Length", &cl_header, false, true) && atol(cl_header) > 0 &&
            atol(cl_header) < package->Caller->GetMaxBodySize()) {
            cli->Request.Body = (char*)malloc(atol(cl_header) + 1);
            cli->ReadBufSize +=
              snprintf(cli->Request.Body, atol(cl_header) + 1, &cli->ReadBufferPTR[cli->ReadBufSize]) + 1;
          }
          if (cl_header)
            free(cl_header);
        }

        cli->Request.Method = HttpServer::ParseMethod(method);
        //cli->Request.Method = HttpServer::HttpMethod::GET;

        if (!cli->Request.GetHeaderValue("Host", &voidpointer, true, false)) {
          cli->Response.CopyFrom(&package->Caller->Response400);
          cli->Response.SetBody("\"Host\" header field not set");
          // goto buildResponseFromPreset; // (?)
        }

        // printf("Serving '%s' to client. ",cli->Request.URI); fflush (stdout);

        HttpServer::http_endpoint_t* endpointPtr = nullptr;

        if (strcmp(cli->Request.Version, "HTTP/1.1") &&
          strcmp(cli->Request.Version, "HTTP/1.0" /* we lie >:) */)) {
          // invalid HTTP version
          cli->Response.CopyFrom(&package->Caller->Response505);
        } else {
          if (!strcmp(cli->Request.Version, "HTTP/1.0")) cli->Keepalive = false;
          else cli->Keepalive = true;
          // TODO: hashmap
          for (short j = 0; (!endpointPtr) && j < FPX_HTTP_ENDPOINTS; j++) {
            if (!strcmp(package->Endpoints[j].URI, cli->Request.URI))
              endpointPtr = &package->Endpoints[j];
          }
          if (!(package->Caller->GetOptions() & HttpServer::ManualWebSocket)) {
            char* connHeader = nullptr;
            cli->Request.GetHeaderValue("Connection", &connHeader);

            char* upHeader = nullptr;
            cli->Request.GetHeaderValue("Upgrade", &upHeader);

            if (upHeader && ((connHeader) ? fpx_substringindex(connHeader, "upgrade") > -1 : 0)) {
              // we be upgrading :nydance:
              if (!strcmp(
                    upHeader, "websocket")) {  // && keyHeader && (!strcmp(verHeader, "13")) &&
                // package->Caller->Mode != HttpServer::ServerType::HttpOnly
                // request wants to upgrade to websocket! poggies!
                char* keyHeader = nullptr;
                cli->Request.GetHeaderValue("Sec-WebSocket-Key", &keyHeader, false);

                char* verHeader = nullptr;
                cli->Request.GetHeaderValue("Sec-WebSocket-Version", &verHeader);

                char* protHeader = nullptr;
                cli->Request.GetHeaderValue("Sec-WebSocket-Protocol", &protHeader);

                if (keyHeader && ((verHeader) ? !strcmp(verHeader, "13") : false) &&
                  package->Caller->Mode != HttpServer::ServerType::HttpOnly) {
                  cli->Response.CopyFrom(&package->Caller->Response101);
                  char* accept_part_1 =
                    fpx_substr_replace(keyHeader, "==", "==258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
                  char* accept_part_2 = (char*)calloc(21, 1);
                  fpx_sha1_digest(
                    accept_part_1, fpx_getstringlength(accept_part_1), accept_part_2, 0);
                  //  for (uint8_t i = 0; i < 20; i++) {
                  //    printf("%hhu ", ((uint8_t*)accept_part_2)[i]);
                  //  }
                  //  printf("\n"); fflush(stdout);

                  char* accept_part_3 = fpx_base64_encode(accept_part_2, 20);
                  char* accept_part_4 = (char*)calloc(51, 1);
                  strcpy(accept_part_4, "Sec-WebSocket-Accept: ");
                  strncat(accept_part_4, accept_part_3, 28);
                  cli->Response.AddHeader(accept_part_4);
                  free(accept_part_1);
                  free(accept_part_2);
                  free(accept_part_3);
                  free(accept_part_4);
                } else if (package->Caller->Mode == HttpServer::ServerType::HttpOnly) {
                  // 501 - Not Implemented |
                  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/501 because server
                  // does not support WebSocket
                  cli->Response.CopyFrom(&package->Caller->Response501);
                  cli->WsFail = true;
                } else {
                  // 400 - Bad Request |
                  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/400 because missing
                  // key or wrong version (should be 13)
                  cli->Response.CopyFrom(&package->Caller->Response400);
                  cli->Response.SetBody(
                    "Could not upgrade request because of incorrect request header composition.\nRefer to [RFC-6455] for information about the required request headers and their validity.");
                  cli->WsFail = true;
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
          if (!endpointPtr) {
            cli->Response.CopyFrom(&package->Caller->Response404);
          } else if (!(cli->Request.Method & endpointPtr->AllowedMethods)) {
            cli->Response.CopyFrom(&package->Caller->Response405);
          } else if (!cli->Response.Finalized) {
            endpointPtr->Callback(&cli->Request, &cli->Response);
          }
          if (cli->Response.Headers) {
            char* lowercaseHeaders = fpx_string_to_lower(cli->Response.Headers, true);
            if (!strcmp(cli->Response.Code, "101") &&
              fpx_substringindex(lowercaseHeaders, "sec-websocket-accept: ") > -1 &&
              fpx_substringindex(lowercaseHeaders, "connection: upgrade") > -1)
              cli->WsUpgrade = true;
            free(lowercaseHeaders);
          }
        }

        if (!fpx_getstringlength(cli->Response.Code))
          cli->Response.SetCode("200");
        if (!fpx_getstringlength(cli->Response.Status))
          cli->Response.SetStatus("OK");

      buildResponseFromPreset:

        sprintf(cli->Response.Version, "HTTP/1.1");

        if (cli->WsUpgrade) {
          cli->Response.AddHeader("Upgrade: websocket");
        }

        char* connectionHeader = nullptr;

        if (!cli->WsUpgrade && cli->Request.GetHeaderValue("Connection", &connectionHeader) &&
          cli->Response.Headers) {
          char* respHeadersLower = fpx_string_to_lower(cli->Response.Headers, 1);
          if (fpx_substringindex(respHeadersLower, "connection: ") == -1 &&
            !strcmp(connectionHeader, "close")) {
            cli->Keepalive = false;
          } else cli->Keepalive = true;
          free(respHeadersLower);
        }

        if (connectionHeader)
          free(connectionHeader);

        short amountWritten;

        if (!cli->WsUpgrade)
          if (!cli->Keepalive) {
            cli->Response.AddHeader("Connection: close");
          } else {
            cli->Response.AddHeader("Connection: keep-alive");
            cli->Response.AddHeader("Keep-Alive: timeout=" STR(FPX_HTTP_KEEPALIVE));
          }

        if (cli->Response.Headers &&
          fpx_substringindex(cli->Response.Headers, "Content-Length: ") == -1) {
          char tempHeaderBuf[256];
          int blen = cli->Response.GetBodyLength();
          sprintf(tempHeaderBuf, "Content-Length: %d", blen);
          cli->Response.AddHeader(tempHeaderBuf);
        }
        const char* const defHeaders = package->Caller->GetDefaultHeaders();
        const char* const respHeaders = cli->Response.Headers;
        const char* const respBody = cli->Response.Body;
        amountWritten = snprintf(writeBuffer,
          FPX_HTTP_WRITE_BUF,
          "%s %s %s\r\n",
          cli->Response.Version,
          cli->Response.Code,
          cli->Response.Status);
        memcpy(writeBuffer + amountWritten, defHeaders, fpx_getstringlength(defHeaders));
        amountWritten += fpx_getstringlength(defHeaders);
        if (respHeaders) {
          memcpy(writeBuffer + amountWritten, respHeaders, fpx_getstringlength(respHeaders));
          amountWritten += fpx_getstringlength(respHeaders);
          free(cli->Response.Headers);
        }
        memcpy(writeBuffer + amountWritten, "\r\n", 2);
        amountWritten += 2;
        if (respBody) {
          memcpy(writeBuffer + amountWritten, respBody, fpx_getstringlength(respBody));
          amountWritten += fpx_getstringlength(respBody);
          free(cli->Response.Body);
        }

        if (cli->Response.Code[0] == '1' || cli->Response.Code[0] == '2' ||
          cli->Response.Code[0] == '3') {
          time(&cli->LastActiveSeconds);
        }

        memset(&cli->Request, 0, sizeof(cli->Request));
        memset(&cli->Response, 0, sizeof(cli->Response));

        endpointPtr = nullptr;

        if (cli->WsUpgrade) {
          short lowestCount = FPX_WS_MAX_CLIENTS;
          short lowestIndex = 1;
          for (short j = 0; j < package->Caller->WsThreads; j++) {
            if (package->Caller->WebsocketThreads[j].ClientCount < lowestCount) {
              lowestCount = package->Caller->WebsocketThreads[j].ClientCount;
              lowestIndex = i;
            }
          }
          HttpServer::websocket_threadpackage_t* wsThreadPTR =
            &package->Caller->WebsocketThreads[lowestIndex];
          // vvv this could go bad (infinite yield?)
          pthread_mutex_lock(&package->Caller->WebsocketThreads[lowestIndex].TalkingStick);
          // ^^^
          // printf("Upgrading to WebSocket\n");
          send(pfd->fd, writeBuffer, amountWritten, 0);
          // send(pfd->fd, "\x81\x03\x61\x62\x63", 5, 0); // abc text frame
          wsThreadPTR->PollFDs[wsThreadPTR->ClientCount] = *pfd;
          wsThreadPTR->Clients[wsThreadPTR->ClientCount].LastActiveSeconds = time(NULL);
          wsThreadPTR->Clients[wsThreadPTR->ClientCount].FileDescriptor =
            wsThreadPTR->PollFDs[wsThreadPTR->ClientCount].fd;
          wsThreadPTR->ClientCount++;
          package->HandleDisconnect(i, false);
          pthread_cond_signal(&wsThreadPTR->Condition);
          pthread_mutex_unlock(&wsThreadPTR->TalkingStick);

          cli->Keepalive = false;
        } else {
          send(pfd->fd, writeBuffer, amountWritten, 0);
          // printf("\n");
        }

        handled++;

        memset(writeBuffer, 0, sizeof(writeBuffer));
        amountWritten = 0;

        if (!cli->Keepalive) {
          if (!cli->WsUpgrade) {
            package->HandleDisconnect(i);
          } else {
            for (int j = i; j < FPX_WS_MAX_CLIENTS; j++) {
              if (j + 1 < FPX_WS_MAX_CLIENTS) {
                package->PollFDs[j] = package->PollFDs[j + 1];
                if (package->PollFDs[j + 1].fd == 0)
                  break;
              } else {
                memset(&package->PollFDs[j], 0, sizeof(pollfd));
                memset(&package->Clients[j], 0, sizeof(HttpServer::http_client_t));
                break;
              }
            }
            package->ClientCount--;
          }
        }

        if (cli->ReadBufferPTR)
          free(cli->ReadBufferPTR);
        {
          time_t tempTime = cli->LastActiveSeconds;
          memset(cli, 0, sizeof(HttpServer::http_client_t));
          cli->LastActiveSeconds = tempTime;
        }
      }
      if (handled) {
        j++;
      }
    }

    pthread_mutex_unlock(&package->TalkingStick);
  }
}

/**
 * Takes a pointer to an fpx::websocket_threadpackage_t object.
 */
static void* WebSocketThread(void* threadpack) {
  if (!threadpack)
    return nullptr;
  HttpServer::websocket_threadpackage_t* package =
    (HttpServer::websocket_threadpackage_t*)threadpack;

  uint16_t metadata;
  uint16_t bigendianChecker = 1;
  bool bigendian = (*(char*)&bigendianChecker) ? false : true;
  uint8_t writeBuffer[FPX_WS_WRITEBUF];
  bool locked = false;

  while (1) {
    // printf("locking\n");
    pthread_mutex_lock(&package->TalkingStick);
    // printf("[thread 0x%x] ws clients: %d\n", package->Thread, package->ClientCount);
    if (!package->ClientCount)
      pthread_cond_wait(&package->Condition, &package->TalkingStick);

    short clientsReady = poll(package->PollFDs, package->ClientCount, 1000);
    // printf("Clients ready on thread %d: %d\n", package->Thread, clientsReady);

    int j = 0;
    for (short i = 0; i < package->ClientCount && j < clientsReady; i++) {
      int handled = 0;
      bool disconnect = false;
      // handle polled clients here ^_^
      if (package->PollFDs[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        // bad client connection OR client hung up
        package->HandleDisconnect(i);
        handled++;
        j++;
        continue;
      }
      if (package->PollFDs[i].revents & POLLIN) {
        // receive client data
        ssize_t bytesRead = read(package->PollFDs[i].fd, (char*)&metadata, 2);
        if (!bigendian)
          fpx_endian_swap(&metadata, 2);
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
              } else
                len = firstLen;
              if (firstLen == 127) {
                len &= 0x7fffffffffffffff;
              }
              if (!bigendian)
                fpx_endian_swap(&len, firstLen == 127 ? 8 : firstLen == 126 ? 2 : 1);
            }

            if (!read(package->PollFDs[i].fd, &mask_key, 4)) {
              handled++;
              break;
            }

            if (len > FPX_WS_READBUF) {
              char closeBuf[64] = { 0 };
              snprintf(closeBuf, 63, "disconnect_body_over_%hubytes", FPX_WS_READBUF);
              package->SendClose(i, 1009, bigendian, (uint8_t*)closeBuf);
            }

            if (opcode >= 0x8) {
              // control frame
              if (!fin || len > 125) {
                if (len) {
                  package->Clients[i].ReadBufferPTR = (!package->Clients[i].ReadBufferPTR) ?
                    (uint8_t*)malloc(len) :
                    (uint8_t*)realloc(package->Clients[i].ReadBufferPTR, len);
                  read(package->PollFDs[i].fd, package->Clients[i].ReadBufferPTR, len);
                  free(package->Clients[i].ReadBufferPTR);
                  package->Clients[i].ReadBufferPTR = nullptr;
                }
                handled++;
                break;
              }
              ssize_t ctlBuffer_read =
                read(package->PollFDs[i].fd, package->Clients[i].ControlReadBuffer, len);
              if (ctlBuffer_read) {
                for (int j = 0; j < len; j++) {
                  package->Clients[i].ControlReadBuffer[j] ^= ((uint8_t*)&mask_key)[j % 4];
                }
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
                    package->SendClose(
                      i, htons(*((uint16_t*)package->Clients[i].ControlReadBuffer)), bigendian);
                  }

                  package->Clients[i].Flags |= FPX_WS_RECV_CLOSE;
                  handled++;
                  disconnect = true;
                  break;

                case 0x9:
                  // ping
                  if (len)
                    memcpy(
                      writeBuffer + snprintf((char*)writeBuffer, FPX_WS_WRITEBUF, "\x8a%lu", len),
                      package->Clients[i].ControlReadBuffer,
                      len);
                  send(package->PollFDs[i].fd, writeBuffer, 2 + len, 0);
                  handled++;
                  break;

                case 0xA:
                  // pong

                  break;
              }

              if (package->Callback)
                package->Callback(&package->Clients[i],
                  metadata,
                  len,
                  mask_key,
                  package->Clients[i].ControlReadBuffer);
              memset(package->Clients[i].ControlReadBuffer,
                0,
                sizeof(package->Clients[i].ControlReadBuffer));
            } else {
              // data frame

              size_t oldsize = package->Clients[i].ReadBufSize;
              if (package->Clients[i].ReadBufferPTR && package->Clients[i].Fragmented) {
                package->Clients[i].ReadBufferPTR =
                  (uint8_t*)realloc(package->Clients[i].ReadBufferPTR,
                    (package->Clients[i].ReadBufSize += len + 1) & FPX_WS_READBUF);
              } else if (package->Clients[i].ReadBufferPTR) {
                package->Clients[i].ReadBufferPTR =
                  (uint8_t*)realloc(package->Clients[i].ReadBufferPTR,
                    (package->Clients[i].ReadBufSize = len + 1) & FPX_WS_READBUF);
              } else
                package->Clients[i].ReadBufferPTR =
                  (uint8_t*)malloc((package->Clients[i].ReadBufSize = len + 1) & FPX_WS_READBUF);
              ssize_t frameBodyLengthRead =
                read(package->PollFDs[i].fd, package->Clients[i].ReadBufferPTR + oldsize, len);
              package->Clients[i].ReadBufferPTR[len] = 0;

              if ((package->Clients[i].Fragmented && opcode) ||
                !(package->Clients[i].Fragmented || opcode)) {
                // flush this frame, because it's:
                // - not a continuation frame where expected
                // - a continuation frame where unexpected
                uint8_t tmpBuf[len];
                read(package->PollFDs[i].fd, tmpBuf, len);
                handled++;
                break;
              }


              // unmask client payload
              for (int j = 0; j < len; j++) {
                package->Clients[i].ReadBufferPTR[j] ^= ((uint8_t*)&mask_key)[j % 4];
              }

              // strip readbuffer of \r or \r\n
              if (package->Clients[i].ReadBufferPTR[frameBodyLengthRead - 1] == '\n') {
                if (package->Clients[i].ReadBufferPTR[frameBodyLengthRead - 2] == '\r') {
                  package->Clients[i].ReadBufferPTR[frameBodyLengthRead - 2] = 0;
                }
                package->Clients[i].ReadBufferPTR[frameBodyLengthRead - 1] = 0;
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
              if (package->Callback)
                package->Callback(
                  &package->Clients[i], metadata, len, mask_key, package->Clients[i].ReadBufferPTR);
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
        if (disconnect)
          package->HandleDisconnect(i);
      }
    }

    for (int i = 0; i < package->ClientCount; i++) {
      // printf("Time now: %u | Client last active: %u | Difference: %u\n", time(NULL)/60,
      // package->Clients[i].LastActiveSeconds/60, time(NULL)/60 -
      // package->Clients[i].LastActiveSeconds/60);
      if ((package->Caller->GetWebSocketTimeout()) &&
        (time(NULL) / 60 - package->Clients[i].LastActiveSeconds / 60) >
          package->Caller->GetWebSocketTimeout()) {
        uint8_t message[123];
        snprintf((char*)message,
          123,
          "disconnect_idle_%hu_minutes",
          package->Caller->GetWebSocketTimeout());
        package->SendClose(i, 1000, bigendian, message);
        package->Clients[i].PendingClose = true;
      }
    }
    // printf("unlocking\n");
    pthread_mutex_unlock(&package->TalkingStick);
    usleep(5000);
  }
}

/**
 * Takes an instance of fpx::HttpServer as an argument
 * Kills idle keepalive connections after the timeout has passed
 */
static void* HttpKillerThread(void* hs) {
  if (!hs)
    return NULL;

  HttpServer* httpServ = (HttpServer*)hs;
  uint8_t i = 0;
  while (1) {
    // usleep( 1000000 / httpServ->HttpThreads);
    HttpServer::http_threadpackage_t* thread = &httpServ->RequestHandlers[i];
    // debug print!
    //printf("clients: %d\n", thread->ClientCount);
    //printf("time: %ld | lastactive: %ld | diff: %ld\n", time(NULL), thread->Clients[j].LastActiveSeconds, time(NULL) - thread->Clients[j].LastActiveSeconds);
    if (thread->ClientCount == 0)
      continue;
    for (uint8_t j = 0; j < thread->ClientCount; j++) {
      // compare time for keepalive
      if (thread->Clients[j].LastActiveSeconds > 0 && thread->Clients[j].Keepalive &&
        time(NULL) - thread->Clients[j].LastActiveSeconds > FPX_HTTP_KEEPALIVE) {
        pthread_mutex_lock(&thread->TalkingStick);
        thread->HandleDisconnect(j);
        pthread_mutex_unlock(&thread->TalkingStick);
      }
    }

    i = (i == httpServ->HttpThreads - 1) ? 0 : i + 1;
  }
}


}  // namespace ServerProperties

HttpServer::HttpServer(uint8_t http_threads, uint8_t ws_threads) :
  TcpServer(),
  m_EndpointCount(0),
  RequestHandlers((http_threadpackage_t*)calloc(http_threads, sizeof(http_threadpackage_t))),
  WebsocketThreads(
    (websocket_threadpackage_t*)calloc(ws_threads, sizeof(websocket_threadpackage_t))),
  HttpThreads(http_threads),
  WsThreads(ws_threads),
  m_Endpoints((http_endpoint_t*)calloc(FPX_HTTP_ENDPOINTS, sizeof(http_endpoint_t))),
  m_DefaultHeaders(nullptr),
  m_MaxBodyLen(4096),
  m_Options(0),
  m_WebSocketTimeout(0),
  m_ServerType(ServerType::Both) {
  SetDefaultHeaders("Server: fpxHTTP (" FPX_HTTPSERVER_VERSION ")\r\n");

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
  if (m_DefaultHeaders)
    free(m_DefaultHeaders);
}

const char* HttpServer::GetDefaultHeaders() {
  return m_DefaultHeaders;
}

void HttpServer::SetDefaultHeaders(const char* headers) {
  if (m_DefaultHeaders) {
    free(m_DefaultHeaders);
  }
  if (int len = fpx_getstringlength(headers)) {
    m_DefaultHeaders = (char*)malloc(len);
    memcpy(m_DefaultHeaders, headers, len);
  }
  return;
}

uint16_t HttpServer::GetMaxBodySize() {
  return m_MaxBodyLen;
}

void HttpServer::SetMaxBodySize(uint16_t len) {
  m_MaxBodyLen = len;
}

void HttpServer::CreateEndpoint(const char* uri, short methods, http_callback_t endpointCallback) {
  if (m_EndpointCount == FPX_HTTP_ENDPOINTS)
    throw Exception("Maximum endpoint count reached!");

  snprintf(m_Endpoints[m_EndpointCount].URI, 255, "%s", uri);
  m_Endpoints[m_EndpointCount].URI[255] = 0;
  m_Endpoints[m_EndpointCount].Callback = endpointCallback;
  m_Endpoints[m_EndpointCount].AllowedMethods = methods;
  m_EndpointCount++;
  return;
}

void HttpServer::Listen(const char* ip, unsigned short port, ws_callback_t websocketCallback) {

  if (!inet_aton(ip, &(m_SocketAddress4.sin_addr)))
    perror("Invalid IP address");
  m_SocketAddress4 = { AF_INET, htons(port), {} };
  m_Port = port;
  int optvalTrue = 1;

  if (m_ServerType == ServerType::WebSockets)
    throw NotImplementedException("\"WebSockets only\" mode has not yet been implemented...");

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

  // create threads to handle incoming HTTP requests
  for (short i = 0; i < HttpThreads; i++) {
    RequestHandlers[i].Endpoints = m_Endpoints;
    RequestHandlers[i].Caller = this;
    pthread_create(&RequestHandlers[i].Thread,
      NULL,
      ServerProperties::HttpProcessingThread,
      &RequestHandlers[i]);
  }

  // create an HTTP connection-killer thread
  pthread_create(&HttpKillerThread, NULL, ServerProperties::HttpKillerThread, this);

  // create threadpackages for WebSocket threads
  for (short i = 0; i < WsThreads; i++) {
    WebsocketThreads[i].Caller = this;
    WebsocketThreads[i].Callback = websocketCallback;
    pthread_create(
      &WebsocketThreads[i].Thread, NULL, ServerProperties::WebSocketThread, &WebsocketThreads[i]);
  }

  m_IsListening = true;

  printf("HTTPserver listening on %s:%d\n", inet_ntoa(m_SocketAddress4.sin_addr), m_Port);
  // usleep(500000);

  while (m_IsListening) {
    short currentLow = FPX_HTTP_MAX_CLIENTS;
    short lowestThread = 0;
    int newClient = accept(m_Socket4, &m_ClientAddress, &m_ClientAddressSize);
    while (newClient) {
      for (int i = 0; i < HttpThreads; i++) {
        if (RequestHandlers[i].ClientCount < currentLow) {
          currentLow = RequestHandlers[i].ClientCount;
          lowestThread = i;
        }
      }
      while (1) {
        if (!pthread_mutex_trylock(&RequestHandlers[lowestThread].TalkingStick)) {
          // printf("Sending client to thread %d (0-indexed)!\n", i);
          if (RequestHandlers[lowestThread].ClientCount > (FPX_HTTP_MAX_CLIENTS - 1))
            break;
          pollfd* theFD =
            &RequestHandlers[lowestThread].PollFDs[RequestHandlers[lowestThread].ClientCount];
          theFD->fd = newClient;
          theFD->events = POLLIN;
          RequestHandlers[lowestThread].ClientCount++;
          pthread_cond_signal(&RequestHandlers[lowestThread].Condition);
          pthread_mutex_unlock(&RequestHandlers[lowestThread].TalkingStick);
          newClient = 0;
          break;
        }
      }
    }
    // debug print!
    //printf("(just accepted a connection) Connected clients: %d\n", ++count);
  }
}

void HttpServer::ListenSecure(
  const char* keypath, const char* certpath, ServerType mode, ws_callback_t websocketCallback) {
  throw NotImplementedException();
}

void HttpServer::Close() {
  if (!m_IsListening)
    return;

  close(m_Socket4);
  m_Socket4 = 0;

  for (short i = 0; i < HttpThreads; i++) {
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
    if (Headers)
      Headers = (char*)realloc(Headers, otherHLen);
    else
      Headers = (char*)malloc(otherHLen);
    memcpy(Headers, other->Headers, otherHLen);
    m_HeaderLen = otherHLen;
  }
  if (other->Body && otherBLen) {
    if (Body)
      Body = (char*)realloc(Body, otherBLen);
    else
      Body = (char*)malloc(otherBLen);
    memcpy(Body, other->Body, otherBLen);
    m_BodyLen = otherBLen;
  }
  Finalized = true;
}

bool HttpServer::http_response_t::SetCode(const char* code) {
  if (fpx_getstringlength(code) != 3)
    return false;
  memcpy(this->Code, code, 4);
  return true;
}

bool HttpServer::http_response_t::SetStatus(const char* status) {
  if (fpx_getstringlength(status) > 31)
    return false;
  memcpy(this->Status, status, fpx_getstringlength(status));
  return true;
}

bool HttpServer::http_response_t::SetHeaders(const char* headers) {
  // printf("Call set!\n");
  m_HeaderLen = fpx_getstringlength(headers);
  char* newAlloc;
  if (m_HeaderLen < 1)
    return false;
  // printf("Old: %x\n", this->Headers);
  if ((this->Headers = (this->Headers) ? (char*)realloc(this->Headers, m_HeaderLen + 1) :
                                         (char*)malloc(m_HeaderLen + 1)) == NULL)
    return false;
  // printf("this->Headers in SetHeaders: 0x%x\n", this->Headers);
  // printf("New: %x\n", newAlloc);
  memcpy(this->Headers, headers, m_HeaderLen);
  this->Headers[m_HeaderLen] = 0;
  return true;
}

bool HttpServer::http_response_t::SetBody(const char* body) {
  m_BodyLen = fpx_getstringlength(body);
  if (m_BodyLen < 1)
    return false;
  this->Body =
    (this->Body) ? (char*)realloc(this->Body, m_BodyLen + 1) : (char*)malloc(m_BodyLen + 1);
  // printf("this->Body in SetBody: 0x%x\n", this->Body);
  memcpy(this->Body, body, m_BodyLen);
  this->Body[m_BodyLen] = 0;

  return true;
}

bool HttpServer::http_request_t::GetHeaderValue(
  const char* headerName, char** resultStore, bool lowercase, bool storeValue) {
  char* header = (char*)malloc(fpx_getstringlength(headerName) + 3);
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
  if ((foundIndex = fpx_substringindex(lowercaseHeaders, header)) > -1) {
    char temp[256] = { 0 };
    int written = strcspn(this->Headers + foundIndex + fpx_getstringlength(headerName) + 1, "\r\n");
    snprintf(temp, written, "%s", this->Headers + foundIndex + fpx_getstringlength(headerName) + 2);
    returnedString = (char*)malloc(written + 1);
    memcpy(returnedString, temp, written);
    returnedString[written] = 0;
  } else {
    returnedString = nullptr;
  }

  free(header);
  free(lowercaseHeaders);
  if (lowercase && returnedString)
    fpx_string_to_lower(returnedString, 0);
  *resultStore = returnedString;
  return foundIndex > -1;
}

int HttpServer::http_response_t::GetHeaderLength() {
  return m_HeaderLen;
}

int HttpServer::http_response_t::GetBodyLength() {
  return m_BodyLen;
}

int HttpServer::http_response_t::AddHeader(const char* newHeader, bool freeOld) {
  // printf("Call add!\n");
  int addedLen = fpx_getstringlength(newHeader);
  char* newAlloc;
  if ((!this->Headers) || freeOld) {
    if (this->Headers) {
      int oldheadLen = fpx_getstringlength(this->Headers);
      char oldHead[oldheadLen + 1];
      strcpy(oldHead, this->Headers);
      char* oldHeadersPtr = this->Headers;
      this->Headers = (char*)malloc(fpx_getstringlength(newHeader) + 2);
      free(oldHeadersPtr);
      strcpy(this->Headers, oldHead);
    } else
      this->Headers = (char*)malloc(fpx_getstringlength(newHeader) + 2);
    // printf("this->Headers in AddHeader: 0x%x\n", this->Headers);
  }
  if (fpx_substringindex(this->Headers, newHeader) < 0) {
    // printf("Old: %x\n", this->Headers);
    if ((this->Headers = (char*)realloc(this->Headers, m_HeaderLen + addedLen + 3)) == NULL)
      return -1;
    // printf("New: %x\n", newAlloc);
  } else
    return -1;
  memcpy(this->Headers + m_HeaderLen, newHeader, addedLen);
  memcpy(this->Headers + m_HeaderLen + addedLen, "\r\n", 3);
  m_HeaderLen += addedLen + 2;
  return (addedLen + 2);
}

void HttpServer::http_threadpackage_t::HandleDisconnect(int clientIndex, bool closeSocket) {
  bool moveMode = false, done = false;
  short index = 0;

  if (PollFDs[clientIndex].fd == 0) {
    // printf("it's been an honor fellas\n");
    // throw IndexOutOfRangeException("There is no HTTP client with that index");
    return;
  }

  // printf("Disconnecting someone\n");

  if (Clients[clientIndex].ReadBufferPTR) {
    free(Clients[clientIndex].ReadBufferPTR);
  }

  memset(&Clients[clientIndex], 0, sizeof(http_client_t));

  if (closeSocket)
    close(PollFDs[clientIndex].fd);

  if (clientIndex == FPX_HTTP_MAX_CLIENTS - 1) {
    memset(&Clients[clientIndex], 0, sizeof(http_client_t));
  } else {
    int i = clientIndex;
    do {
      //printf("%d\n", PollFDs[i].fd);
      memcpy(&Clients[i], &Clients[i+1], sizeof(http_client_t));
      memcpy(&PollFDs[i], &PollFDs[i+1], sizeof(struct pollfd));
      ++i;
    } while (PollFDs[i+1].fd != 0);
  }

  ClientCount--;
  // debug print!
  //printf("just killed one | Connected clients: %d\n", --count);
}

void HttpServer::websocket_client_t::Ping() {
  websocket_frame_t frame;
  frame.SetBit(128);
  frame.SetOpcode(0x09);
  frame.Send(FileDescriptor);
}

void HttpServer::websocket_frame_t::SetBit(uint8_t bit, bool value) {
  if (value)
    m_MetaByte |= bit;
  else
    m_MetaByte &= ~bit;
}

void HttpServer::websocket_frame_t::SetOpcode(uint8_t opcode) {
  m_MetaByte |= (opcode & 0x0f);
}

bool HttpServer::websocket_frame_t::SetPayload(const char* payload, uint16_t len) {
  if (!payload)
    return false;
  if (!len)
    len = fpx_getstringlength(payload);

  if (m_PayloadLen >= len && m_Payload)
    memset(&m_Payload[len], 0, m_PayloadLen - len);
  else if (m_Payload)
    m_Payload = (char*)realloc(m_Payload, len);
  else
    m_Payload = (char*)malloc(len);

  m_PayloadLen = len;

  memcpy(m_Payload, payload, len);

  return true;
}

void HttpServer::websocket_frame_t::Send(int fd) {
  if (!fd)
    return;

  uint64_t firstLen = m_PayloadLen;
  int offset = 0;
  if (m_PayloadLen > 125) {
    if (m_PayloadLen < USHRT_MAX)
      firstLen = 126;
    else if (m_PayloadLen < ULLONG_MAX)
      firstLen = 127;
  }
  firstLen &= 0x7f;

  uint64_t payLen = m_PayloadLen;
  uint16_t orderChecker = 1;
  if (firstLen > 125 && *((uint8_t*)&orderChecker))
    fpx_endian_swap(&payLen, (firstLen == 126) ? 2 : 8);

  uint64_t totalLen = 2 + ((firstLen > 125) ? (firstLen == 127) ? 8 : 2 : 0) + m_PayloadLen;

  uint8_t* buf = (uint8_t*)calloc(totalLen, 1);
  memcpy(buf + (offset++), &m_MetaByte, 1);
  memcpy(buf + (offset++), &firstLen, 1);
  if (firstLen > 125) {
    memcpy(buf + offset, &payLen, (firstLen == 126) ? 2 : 8);
    offset += (firstLen == 126) ? 2 : 8;
  }
  memcpy(buf + offset, m_Payload, m_PayloadLen);

  write(fd, buf, totalLen);
  free(buf);
}

void HttpServer::websocket_threadpackage_t::HandleDisconnect(int clientIndex, bool closeSocket) {
  bool moveMode = false, done = false;
  short index = 0;

  if (PollFDs[clientIndex].fd == 0) {
    throw IndexOutOfRangeException("There is no WebSocket client with that index");
    // return;
  }

  if (Clients[clientIndex].ReadBufferPTR) {
    free(Clients[clientIndex].ReadBufferPTR);
    memset(&Clients[clientIndex], 0, sizeof(websocket_client_t));
  }

  close(PollFDs[clientIndex].fd);

  memset(&Clients[clientIndex], 0, sizeof(websocket_client_t));
  memset(&PollFDs[clientIndex], 0, sizeof(pollfd));

  for (int i = clientIndex; i < FPX_WS_MAX_CLIENTS; i++) {
    if (i + 1 < FPX_WS_MAX_CLIENTS) {
      Clients[i] = Clients[i + 1];
      PollFDs[i] = PollFDs[i + 1];
      if (PollFDs[i + 1].fd == 0)
        break;
    } else {
      memset(&PollFDs[i], 0, sizeof(pollfd));
      memset(&Clients[i], 0, sizeof(websocket_client_t));
      break;
    }
  }
  Clients[clientIndex].PendingClose = false;
  ClientCount -= 1;
}

void HttpServer::websocket_threadpackage_t::SendFrame(int index, websocket_frame_t*) {
  throw NotImplementedException();
}

void HttpServer::websocket_threadpackage_t::SendClose(
  int index, uint16_t status, bool bigEndian, uint8_t* message) {
  if (!bigEndian)
    status = htons(status);
  uint8_t buflen = 4;
  uint8_t stringlength = (message) ? (uint8_t)fpx_getstringlength((char*)message) : 0;
  stringlength = (stringlength > 123) ? 123 : stringlength;
  if (message)
    buflen += stringlength;
  uint8_t writebuf[buflen];
  writebuf[0] = 0x88;
  writebuf[1] = buflen - 2;
  ((uint16_t*)writebuf)[1] = status;
  if (message) {
    memcpy(&writebuf[4], (uint8_t*)message, stringlength);
  }
  send(PollFDs[index].fd, writebuf, buflen, 0);
  Clients[index].PendingClose = true;
}

HttpServer::HttpMethod HttpServer::ParseMethod(const char* method) {
  //printf("CALLED!\n");
  if (!strcmp(method, "GET"))
    return HttpServer::GET;
  if (!strcmp(method, "HEAD"))
    return HttpServer::HEAD;
  if (!strcmp(method, "POST"))
    return HttpServer::POST;
  if (!strcmp(method, "PUT"))
    return HttpServer::PUT;
  if (!strcmp(method, "DELETE"))
    return HttpServer::DELETE;
  if (!strcmp(method, "CONNECT"))
    return HttpServer::CONNECT;
  if (!strcmp(method, "OPTIONS"))
    return HttpServer::OPTIONS;
  if (!strcmp(method, "TRACE"))
    return HttpServer::TRACE;
  if (!strcmp(method, "PATCH"))
    return HttpServer::PATCH;
  return HttpServer::NONE;
}

void HttpServer::SetServerType(HttpServer::ServerType type) {
  m_ServerType = type;
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

}  // namespace fpx

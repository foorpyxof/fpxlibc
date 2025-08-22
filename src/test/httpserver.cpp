extern "C" {


#include "../fpx_networking/http/httpserver.h"
#include "../fpx_debug.h"

#include "../fpx_mem/mem.h"
#include "../fpx_networking/http/http.h"
#include "../fpx_networking/http/websockets.h"
#include "../fpx_string/string.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#define IP "0.0.0.0"
#define PORT 8080
}

struct file_to_serve {
    char name[128];
    char* content;
    int file_size;
};

void* sender(void* filedescriptor) {
  int fd = *((int*)filedescriptor);

  while (TRUE) {
    fpx_websocketframe_t out_frame;
    fpx_websocketframe_init(&out_frame);

    out_frame.final = TRUE;
    out_frame.opcode = WEBSOCKET_TEXT;

    const uint8_t msg[] = "websocket looper message!";
    fpx_websocketframe_append_payload(&out_frame, msg, sizeof(msg) - 1);
    int sent_status = fpx_websocketframe_send(&out_frame, fd);
    fpx_websocketframe_destroy(&out_frame);

    if (sent_status != 0) {
      // something went wrong ooops
      char file_and_line[64] = { 0 };
      FPX_LINE_INFO(file_and_line);
      FPX_WARN("sending went wrong: return code %d | %s\n", sent_status, file_and_line);

      break;
    }

    usleep(500000);
  }

  return NULL;
}

void ws_root_callback(
  const fpx_websocketframe_t* in, int fd, const struct sockaddr* client_address) {

  fpx_websocketframe_t out_frame;
  fpx_websocketframe_init(&out_frame);

  out_frame.final = TRUE;
  out_frame.opcode = in->opcode;

  if (NULL != in->payload) {
    // FPX_DEBUG("payload: %s\n", in->payload);
  }

  // uint8_t msg[] = "Message from WS-server: ";
  // fpx_websocketframe_append_payload(&out_frame, msg, sizeof(msg) - 1);

  fpx_websocketframe_append_payload(&out_frame, in->payload, in->payload_length);

  fpx_websocketframe_send(&out_frame, fd);

  fpx_websocketframe_destroy(&out_frame);

  return;
}

void ws_loop_callback(
  const fpx_websocketframe_t* incoming, int fd, const struct sockaddr* client_address) {
  pthread_t t;

  pthread_create(&t, NULL, sender, &fd);

  usleep(10000);
  pthread_detach(t);

  return;
}

void root_callback(const fpx_httprequest_t* reqptr, fpx_httpresponse_t* resptr) {
  fpx_httpresponse_add_header(resptr, "content-type", "text/plain");

  char msg[] = "Hello from fpx_http :D\n";
  fpx_httpresponse_append_body(resptr, msg, sizeof(msg) - 1);

  return;
}

struct file_to_serve files[5];

void source_callback(const fpx_httprequest_t* reqptr, fpx_httpresponse_t* resptr) {

  // fpx_memcpy(files[0].name, "http.h", 7);
  // fpx_memcpy(files[1].name, "http.c", 7);
  // fpx_memcpy(files[2].name, "httpserver.h", 13);
  // fpx_memcpy(files[3].name, "httpserver.c", 13);
  // fpx_memcpy(files[4].name, "test.c", 7);

  for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {

    if (NULL == files[i].content) {
      FILE* fp = fopen(files[i].name, "r");

      if (NULL == fp) {
        perror("fopen()");
        char reason[] = "Internal Server Error";

        resptr->status = 500;
        fpx_memcpy(resptr->reason, reason, sizeof(reason) - 1);

        char body[] = "Failed to open file";
        fpx_httpresponse_append_body(resptr, body, sizeof(body) - 1);

        FPX_ERROR("%s %s", body, files[i].name);

        return;
      }

      fseek(fp, 0, SEEK_END);
      files[i].file_size = ftell(fp);
      files[i].content =
        (char*)mmap(NULL, files[i].file_size, PROT_READ, MAP_SHARED, fileno(fp), 0);
    }
    fpx_httpresponse_append_body(resptr, files[i].content, files[i].file_size);
  }


  fpx_httpresponse_add_header(resptr, "Content-Type", "text/plain");

  char new_body[] = "<h1>Hello from fpx_http!!! :3</h1>";
  // fpx_httpresponse_append_body(resptr, new_body, sizeof(new_body) - 1);

  return;
}

int main() {
  {
    FILE* fpipe = NULL;
    char command[512] = { 0 };

    char pwd[256] = { 0 };
    fpipe = popen("pwd", "r");
    int read_count = read(fileno(fpipe), pwd, sizeof(pwd) - 1);
    pwd[read_count - 1] = 0;
    fclose(fpipe);

    // FPX_DEBUG("%s\n", pwd);

    snprintf(command,
      sizeof(command) - 1,
      "find %s -name \"http.h\" -o -name \"http.c\" -o -name \"httpserver.h\" -o -name \"httpserver.c\" -o -name \"httpserver.cpp\"",
      pwd);

    fpipe = NULL;
    fpipe = popen(command, "r");

    if (NULL == fpipe) {
      perror("popen()");
      exit(EXIT_FAILURE);
    }

    char buffer[256] = { 0 };
    int i = 0;
    while (NULL != fgets(buffer, sizeof(buffer), fpipe) && i < (sizeof(files) / sizeof(*files))) {
      buffer[fpx_getstringlength(buffer) - 1] = 0;
      // FPX_DEBUG("loading filename %s\n", buffer);
      fpx_strcpy(files[i++].name, buffer);
    }

    fclose(fpipe);
  }


  fpx_httpserver_t serv;

  fpx_httpserver_init(&serv, 8, 1, 16);

  fpx_httpserver_create_endpoint(&serv, "/", GET, root_callback);
  fpx_httpserver_create_endpoint(&serv, "/source", GET, source_callback);

  fpx_httpserver_create_ws_endpoint(&serv, "/", ws_root_callback);
  fpx_httpserver_create_ws_endpoint(&serv, "/loop", ws_loop_callback);

  fpx_httpserver_listen(&serv, IP, PORT);
}

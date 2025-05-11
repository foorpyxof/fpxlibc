#include "../../fpx_debug.h"
#include "httpserver_c.h"

#include "../../fpx_mem/mem.h"
#include "http.h"

#include <fcntl.h>
#include <sys/mman.h>

#define IP "0.0.0.0"
#define PORT 8080

struct file_to_serve {
    char name[32];
    char* content;
    int file_size;
};

void root_callback(const fpx_httprequest_t* reqptr, fpx_httpresponse_t* resptr) {
  static struct file_to_serve files[4];

  fpx_memcpy(files[0].name, "http.h", 7);
  fpx_memcpy(files[1].name, "http.c", 7);
  fpx_memcpy(files[2].name, "httpserver_c.h", 15);
  fpx_memcpy(files[3].name, "httpserver_c.c", 15);

  for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {

    if (NULL == files[i].content) {
      FILE* fp = fopen(files[i].name, "r");

      if (NULL == fp) {
        char reason[] = "Internal Server Error";

        resptr->status = 500;
        fpx_memcpy(resptr->reason, reason, sizeof(reason) - 1);

        char body[] = "Failed to open file";
        fpx_httpresponse_append_body(resptr, body, sizeof(body) - 1);

        FPX_ERROR("%s", body);

        continue;
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
  fpx_httpserver_t serv;

  fpx_httpserver_init(&serv, 1, 1, 16);

  fpx_httpserver_create_endpoint(&serv, "/", GET, root_callback);

  fpx_httpserver_listen(&serv, IP, PORT);
}

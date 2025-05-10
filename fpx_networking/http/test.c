#include "httpserver_c.h"
#include "../../fpx_debug.h"
#include <http.h>
#include <stdio.h>

#define IP "0.0.0.0"
#define PORT 8080

void root_callback(fpx_httprequest_t* reqptr, fpx_httpresponse_t* resptr) {
  fpx_httpresponse_add_header(resptr, "YAHALLO", "uwuUWU");

  char new_body[] = "Hello from fpx_http!!! :3";
  fpx_httpresponse_append_body(resptr, new_body, sizeof(new_body) - 1);

  return;
}

int main() {
  fpx_httpserver_t serv;

  fpx_httpserver_init(&serv, 1, 1, 16);

  fpx_httpserver_create_endpoint(&serv, "/", GET, root_callback);

  fpx_httpserver_listen(&serv, IP, PORT);
}

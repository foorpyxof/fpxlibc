#include "httpserver_c.h"

int main() {
  fpx_httpserver_t serv;

  fpx_httpserver_init(&serv, 1, 1, 16);
  fpx_httpserver_set_default_headers(&serv, "abc: def\r\n");
}

#include "httpserver_c.h"

int main() {
  fpx_httpserver_t serv;

  fpx_httpserver_init(&serv, 1, 1, 16);
  fpx_httpserver_set_default_headers(&serv, "   aBc:   deF   \r\nuwu:tESt_he4der\r\n          YIPEE:             this is a HEADER VALUE!                         \r\n");

  fpx_httpserver_listen(&serv, "0.0.0.0", 8080);
}

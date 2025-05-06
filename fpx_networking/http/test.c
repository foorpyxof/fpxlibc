#include "httpserver_c.h"

#include <stdio.h>

int main() {
  fpx_httpserver_t serv;

  fpx_httpserver_init(&serv, 1, 1, 16);
  fpx_httpserver_set_default_headers(&serv, "   aBc:   deF   \r\nsuGMa:gOck\r\n          YIPEE:             this is a HEADER VALUE!                         \r\n");

  char outbuf[128];
  int retval = fpx_httpserver_get_default_headers(&serv, outbuf, sizeof(outbuf));


  printf("%s\n", outbuf);
}

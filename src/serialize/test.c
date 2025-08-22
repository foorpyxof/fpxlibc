#include "serialize/json.h"

int main(void) {

  char test_string[] =
    "    { \"abcdef\" : true, \"test key WOAH\" : {\"ligma\"   : [   12,null,-32        ,  false]}      }";

  Fpx_Json_Entity new_entity = fpx_json_read(test_string, sizeof(test_string) - 1);

  if (new_entity.arena) { }

  return 0;
}

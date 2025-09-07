#include "serialize/json.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {

  if (argc < 2) {
    fprintf(stderr, "requires JSON input file as argument\n");
    return EXIT_FAILURE;
  }

  FILE* json_file = fopen(argv[1], "rb");
  if (NULL == json_file) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  fseek(json_file, 0, SEEK_END);

  long file_size = ftell(json_file);
  if (0 > file_size) {
    perror("ftell()");
    return EXIT_FAILURE;
  }


  char* test_string = (char*)malloc(file_size);

  rewind(json_file);

  if ((unsigned long)file_size > fread(test_string, 1, file_size, json_file)) {
    printf("feof: %d | ferror: %d\n", feof(json_file), ferror(json_file));
    return EXIT_FAILURE;
  }

  fclose(json_file);

  Fpx_Json_Entity new_entity = fpx_json_read(test_string, file_size);

  // printf("%s\n", ((new_entity.isValid) ? "JSON parse valid!" : "JSON parse failed"));

  fpx_json_print(&new_entity);

  fpx_json_destroy(&new_entity);

  free(test_string);

  return 0;
}

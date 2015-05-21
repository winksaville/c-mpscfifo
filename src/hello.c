#include <stdio.h>
#include "msg.h"

int main(int argc, char *argv[]) {
  printf("Hello, World! arc=%d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf(" argv[%d]=%s\n", i, argv[i]);
  }
  return 0;
}

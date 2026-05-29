#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  char input[50];

  printf("$ ");
  scanf("%s", input);
  printf("%s: command not found\n", input);

  return 0;
}

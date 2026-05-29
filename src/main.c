#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);


    char input[100];

    while(1){
        printf("$ ");

        fgets(input, 100, stdin);

        input[strlen(input) - 1] = '\0';

        if(strcmp(input, "exit") == 0){
            break;
        }else{
            printf("%s: command not found\n", input);
        }
    }
    

  return 0;
}

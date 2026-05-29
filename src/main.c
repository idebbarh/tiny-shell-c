#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);



    while(1){
        char input[100];
        char* parts[100] = {0};
        size_t pc = 0;
        char* token;

        printf("$ ");

        fgets(input, 100, stdin);
        input[strlen(input) - 1] = '\0';

        token = strtok(input, " ");
        while(token != NULL && pc < sizeof(parts) / sizeof(parts[0])){
            parts[pc++] = token;
            token = strtok(NULL, " ");
        }

        if(pc == 1 && strcmp(parts[0], "exit") == 0){
            break;
        }else if(strcmp(parts[0], "echo") == 0){
            size_t i = 1;

            while(i < pc && parts[i] != NULL){
                printf("%s ", parts[i++]);
            }
            printf("\n");
        }else{
            printf("%s: command not found\n", input);
        }
    }
    
    return 0;
}

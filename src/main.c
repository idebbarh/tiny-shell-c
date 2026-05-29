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

        if(strlen(input) > 0){

            token = strtok(input, " ");
            while(token != NULL && pc < sizeof(parts) / sizeof(parts[0])){
                parts[pc++] = token;
                token = strtok(NULL, " ");
            }

            if(strcmp(input, "exit") == 0){
                break;
            }else if(strcmp(parts[0], "echo") == 0){
                size_t i = 1;

                while(i < pc){
                    printf("%s ", parts[i++]);
                }

                if(i > 1){
                    printf("\n");
                }
            }else if(strcmp(parts[0], "type") == 0){
                size_t i = 1;

                while(i < pc){
                    char* cmd = parts[i++];

                    if(strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "type") == 0){
                        printf("%s is a shell builtin\n", cmd);
                    }else {
                        printf("%s: not found\n", cmd);
                    }
                }
            }else{
                printf("%s: command not found\n", input);
            }
        }
    }
    
    return 0;
}

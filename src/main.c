#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#define PATH_SEP ";"
#define get_cwd _getcwd
#else
#include <unistd.h>
#define PATH_SEP ":"
#define get_cwd getcwd
#endif

int lookup_program(char *cmd, char full_path[1024], size_t full_path_size) {
  // get the PATH
  char *path_value = strdup(getenv("PATH"));
  char *subpath;
  int is_found = 0;

  // split it by : if it's linux or ; if it's win.
  subpath = strtok(path_value, PATH_SEP);

  // loop over the subpaths
  while (subpath != NULL) {
    // open the dir
    DIR *dir = opendir(subpath);

    if (dir != NULL) {
      struct dirent *entry;

      // read the dir and get all it's files and dirs.
      while ((entry = readdir(dir)) != NULL) {
        // check if the current file is the cmd
        if (strcmp(entry->d_name, cmd) == 0) {
          // get teh full path of the file.
          snprintf(full_path, full_path_size, "%s/%s", subpath, entry->d_name);

          // check if it's excutable.
          if (access(full_path, X_OK) == 0) {
            is_found = 1;
            break;
          }
        }

        full_path[0] = '\0';
      }

      closedir(dir);
    }

    if (is_found)
      break;

    subpath = strtok(NULL, PATH_SEP);
  }

  free(path_value);

  return is_found;
}

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while (1) {
    char input[100];
    char *parts[100] = {0};
    size_t pc = 0;
    char *token;

    printf("$ ");

    // get user input
    fgets(input, 100, stdin);
    input[strlen(input) - 1] = '\0';

    // check if not empty
    if (strlen(input) > 0) {
      // split it by space and store the parts in parts
      char input_cpy[100];
      strcpy(input_cpy, input);

      token = strtok(input_cpy, " ");
      while (token != NULL && pc < sizeof(parts) / sizeof(parts[0])) {
        parts[pc++] = token;
        token = strtok(NULL, " ");
      }

      // if the input is exit exit the loop
      if (strcmp(input, "exit") == 0) {
        break;

        // check if the first part is echo cmd
      } else if (strcmp(parts[0], "echo") == 0) {
        size_t i = 1;

        // print all the sentences after the echo cmd
        while (i < pc) {
          printf("%s ", parts[i++]);
        }

        printf("\n");

        // check if it's a type cmd
      } else if (strcmp(parts[0], "type") == 0) {
        size_t i = 1;

        // loop over the cmds after it.
        while (i < pc) {
          char *cmd = parts[i++];

          // check if the cmd is builtin.
          if (strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 ||
              strcmp(cmd, "type") == 0 || strcmp(cmd, "pwd") == 0) {
            printf("%s is a shell builtin\n", cmd);

            // chearch for the cmd in the PATH.
          } else {
            char full_path[1024];

            if (lookup_program(cmd, full_path, sizeof(full_path))) {
              printf("%s is %s\n", cmd, full_path);
            } else {
              printf("%s: not found\n", cmd);
            }
          }
        }
      } else if (strcmp(parts[0], "pwd") == 0) {
        char cwd[PATH_MAX];

        if (get_cwd(cwd, sizeof(cwd)) != NULL) {
          printf("%s\n", cwd);
        } else {
          printf(
              "ERROR: Could not get the current working dir from getcwd()\n");
          return 1;
        }
      } else if (strcmp(parts[0], "cd") == 0) {
        char *path = strdup(parts[1] == NULL || strcmp(parts[1], "~") == 0
                                ? getenv("HOME")
                                : strdup(parts[1]));

        if (chdir(path) == -1 && errno == ENOENT) {
          printf("cd: %s: No such file or directory\n", path);
        }

        free(path);
      } else {
        char full_path[1024] = {0};

        if (lookup_program(parts[0], full_path, sizeof(full_path))) {
          system(input);
        } else {
          printf("%s: command not found\n", input);
        }
      }
    }
  }

  return 0;
}

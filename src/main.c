#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stddef.h>
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

#define INPUT_MAX_SIZE 100

void free_input_parts(char *parts[INPUT_MAX_SIZE], size_t parts_size) {
  for (size_t i = 0; i < parts_size; i++) {
    char *part = parts[i];
    if (part != NULL) {
      free(part);
    }
  }
}

size_t input_parser(const char input[INPUT_MAX_SIZE],
                    char *parts[INPUT_MAX_SIZE]) {
  const size_t input_size = strlen(input);

  char token_buffer[INPUT_MAX_SIZE] = {0};
  size_t is_single_quoting = 0;
  size_t is_double_quoting = 0;
  size_t is_quoting = 0;
  size_t input_c = 0;
  size_t token_c = 0;
  size_t parts_c = 0;

  while (input_c < input_size) {
    // get the current char
    char current_char = input[input_c++];

    if (current_char == '\'') {
      if (is_single_quoting) {
        // mark as out of quoting
        is_single_quoting = 0;
        // before saving the token check if the token not empty and the next is
        // either the end or there is space between the curent one and the next
        // token
        if (strlen(token_buffer) > 0 &&
            (input_c + 1 >= input_size ||
             ((input_c + 1 < input_size) && (input[input_c + 1]) == ' '))) {
          parts[parts_c++] = strdup(token_buffer);
          token_buffer[0] = '\0';
          token_c = 0;
        }

        // if it's double quoting save the single quote as normal char
      } else if (is_double_quoting) {
        token_buffer[token_c++] = current_char;
        token_buffer[token_c] = '\0';
      } else {
        is_single_quoting = 1;
      }

    } else if (current_char == '"') {
      if (is_double_quoting) {
        is_double_quoting = 0;
        // before saving the token check if the token not empty and the next is
        // either the end or there is space between the curent one and the next
        // token
        if (strlen(token_buffer) > 0 &&
            (input_c + 1 >= input_size ||
             ((input_c + 1 < input_size) && (input[input_c + 1]) == ' '))) {
          parts[parts_c++] = strdup(token_buffer);
          token_buffer[0] = '\0';
          token_c = 0;
        }
        // if it's signle quoting save the double quote as normal char
      } else if (is_single_quoting) {
        token_buffer[token_c++] = current_char;
        token_buffer[token_c] = '\0';
      } else {
        is_double_quoting = 1;
      }
    } else if (current_char == ' ') {
      // if we are inside single quotes then trait the empty space as normal
      // char and save it inside the current token buffer
      if (is_single_quoting || is_double_quoting) {
        token_buffer[token_c++] = current_char;
        token_buffer[token_c] = '\0';
        // else check if the token is not empty then save the token
      } else if (strlen(token_buffer) > 0) {
        parts[parts_c++] = strdup(token_buffer);
        token_buffer[0] = '\0';
        token_c = 0;
      }
    } else if (current_char == '\\') {
      if (is_single_quoting) {
        // save the backslash as a normal char if it's inside a single quote
        token_buffer[token_c++] = current_char;

      } else if (input_c < input_size) {

        // move to the next char and save it as normal char whatever it is
        token_buffer[token_c++] = input[input_c++];
      }

      token_buffer[token_c] = '\0';
      // else save the char inside the current token buffer
    } else {
      token_buffer[token_c++] = current_char;
      token_buffer[token_c] = '\0';
    }
    //---------------------
  }

  // check if there is unsaved token
  if (strlen(token_buffer) > 0) {
    parts[parts_c++] = strdup(token_buffer);
  }

  return parts_c;
}

int lookup_program(char *cmd, char full_path[PATH_MAX]) {
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
          snprintf(full_path, PATH_MAX, "%s/%s", subpath, entry->d_name);

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
    char input[INPUT_MAX_SIZE];
    char *parts[INPUT_MAX_SIZE] = {0};

    printf("$ ");

    // get user input
    fgets(input, INPUT_MAX_SIZE, stdin);
    input[strlen(input) - 1] = '\0';

    // check if not empty
    if (strlen(input) > 0) {
      // split it by space and store the parts in parts

      size_t parts_size = input_parser(input, parts);

      // if the input is exit exit the loop
      if (strcmp(input, "exit") == 0) {
        free_input_parts(parts, parts_size);
        break;
        // check if the first part is echo cmd
      } else if (strcmp(parts[0], "echo") == 0) {
        size_t i = 1;

        // print all the sentences after the echo cmd
        while (i < parts_size) {
          printf("%s ", parts[i++]);
        }

        printf("\n");

        // check if it's a type cmd
      } else if (strcmp(parts[0], "type") == 0) {
        size_t i = 1;

        // loop over the cmds after it.
        while (i < parts_size) {
          char *cmd = parts[i++];

          // check if the cmd is builtin.
          if (strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 ||
              strcmp(cmd, "type") == 0 || strcmp(cmd, "pwd") == 0) {
            printf("%s is a shell builtin\n", cmd);

            // chearch for the cmd in the PATH.
          } else {
            char full_path[PATH_MAX];

            if (lookup_program(cmd, full_path)) {
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

          free_input_parts(parts, parts_size);
          printf(
              "ERROR: Could not get the current working dir from getcwd()\n");
          return 1;
        }
      } else if (strcmp(parts[0], "cd") == 0) {
        // set the path to the home if the user prvoide not second argument to
        // the cd or he uses ~
        char *path = strdup(parts[1] == NULL || strcmp(parts[1], "~") == 0
                                ? getenv("HOME")
                                : strdup(parts[1]));

        // check if the path is exist.
        if (chdir(path) == -1 && errno == ENOENT) {
          printf("cd: %s: No such file or directory\n", path);
        }

        free(path);
      } else {
        char full_path[PATH_MAX] = {0};

        if (lookup_program(parts[0], full_path)) {
          system(input);
        } else {
          printf("%s: command not found\n", input);
        }
      }

      free_input_parts(parts, parts_size);
    }
  }

  return 0;
}

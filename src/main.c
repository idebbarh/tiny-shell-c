#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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
#define OUTPUT_MAX_SIZE 1000

int write_to_file(char input[OUTPUT_MAX_SIZE], const char *file_path) {
  FILE *fptr = fopen(file_path, "w+");

  if (fptr == NULL) {
    return 1;
  }

  fprintf(fptr, "%s", input);

  fclose(fptr);

  return 0;
}

int check_for_redirect(char *parts[INPUT_MAX_SIZE], size_t *parts_size,
                       char redirect_file_path[PATH_MAX]) {

  size_t ps = *parts_size;

  for (size_t i = 0; i < ps; i++) {
    char *part = parts[i];

    if (strcmp(part, ">") == 0 ||
        (i + 1 < ps && strcmp(part, "1") && strcmp(parts[i + 1], ">"))) {
      if (i + 1 < ps) {
        snprintf(redirect_file_path, PATH_MAX, "%s", parts[i + 1]);
      }

      (*parts_size)--;
      for (size_t j = i; j < ps; j++) {
        parts[j] = NULL;
        (*parts_size)--;
      }

      return 1;
    }
  }

  return 0;
}

void free_input_parts(char *parts[INPUT_MAX_SIZE], const size_t parts_size) {
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
    char output[OUTPUT_MAX_SIZE] = {0};
    char *parts[INPUT_MAX_SIZE] = {0};

    printf("$ ");

    // get user input
    fgets(input, INPUT_MAX_SIZE, stdin);
    input[strlen(input) - 1] = '\0';

    // check if not empty
    if (strlen(input) > 0) {
      // split it by space and store the parts in parts
      size_t parts_size = input_parser(input, parts);
      char redirect_file_path[PATH_MAX];

      int is_redirect =
          check_for_redirect(parts, &parts_size, redirect_file_path);

      // if the input is exit exit the loop
      if (strcmp(input, "exit") == 0) {
        free_input_parts(parts, parts_size);
        break;
        // check if the first part is echo cmd
      } else if (strcmp(parts[0], "echo") == 0) {
        size_t i = 1;
        size_t output_len = strlen(output);

        // print all the sentences after the echo cmd
        while (i < parts_size) {
          snprintf(output + output_len, sizeof(output) - output_len, "%s ",
                   parts[i++]);

          output_len = strlen(output);
        }

        snprintf(output + output_len, sizeof(output) - output_len, "\n");

        // check if it's a type cmd
      } else if (strcmp(parts[0], "type") == 0) {
        size_t i = 1;

        // loop over the cmds after it.
        while (i < parts_size) {
          char *cmd = parts[i++];
          size_t output_len = strlen(output);

          // check if the cmd is builtin.
          if (strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 ||
              strcmp(cmd, "type") == 0 || strcmp(cmd, "pwd") == 0) {

            snprintf(output + output_len, sizeof(output) - output_len,
                     "%s is a shell builtin\n", cmd);

            // chearch for the cmd in the PATH.
          } else {
            char full_path[PATH_MAX];

            if (lookup_program(cmd, full_path)) {
              snprintf(output + output_len, sizeof(output) - output_len,
                       "%s is %s\n", cmd, full_path);
            } else {
              snprintf(output + output_len, sizeof(output) - output_len,
                       "%s: not found\n", cmd);
            }
          }
        }
      } else if (strcmp(parts[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        size_t output_len = strlen(output);

        if (get_cwd(cwd, sizeof(cwd)) != NULL) {
          snprintf(output + output_len, sizeof(output) - output_len, "%s\n",
                   cwd);
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
          int fds[2];
          // make the pipe before the fork so the child inherit it.
          // the pipe works this way, anything get written to the fds[1] you can
          // read it from fds[0]
          pipe(fds);

          pid_t pid = fork();

          // child
          if (pid == 0) {
            // close the read end because child only write
            close(fds[0]);
            // make whatever the fd=1 "stdout" point from terminal to whatever
            // fds[1] points to, this is how we get the the output from the
            // terminal
            dup2(fds[1], 1);
            // close the write end because the child does not need to write
            // anymore
            close(fds[1]);
            // exucute the command
            execvp(full_path, parts);
            // this happend if the execvp fails
            perror("execvp");
            exit(1);
            // parent
          } else {
            // close the write end in the parent because the parent only read.
            close(fds[1]);

            // get the data that the fds[0] points
            FILE *f = fdopen(fds[0], "r");
            char line[256];
            size_t output_len = strlen(output);

            // read it
            while (fgets(line, sizeof(line), f) != NULL) {
              snprintf(output + output_len, sizeof(output) - output_len, "%s",
                       line);
              output_len = strlen(output);
            }

            wait(NULL);
          }
        } else {
          printf("%s: command not found\n", input);
        }
      }

      if (is_redirect) {
        if (write_to_file(output, redirect_file_path)) {
          printf("ERROR: Could open file %s, does it exist?",
                 redirect_file_path);
          return 1;
        }

      } else {
        printf("%s", output);
      }

      free_input_parts(parts, parts_size);
    }
  }

  return 0;
}

#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <readline/readline.h>
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

#define INPUT_CAPACITY 100
#define OUTPUT_CAPACITY 1000
#define COMPLETE_HISTORY_CAPACITY 1000
#define COMPLETE_HISTORY_ELEM_CAPACITY 100

typedef struct {
  char *complete_history[COMPLETE_HISTORY_CAPACITY];
  size_t complete_history_size;
} CompleteCMDState;

static CompleteCMDState complete_cmd_state = {0};
static char **curr_completer_value;

static const char *cmd_names[] = {"exit", "echo",     "type", "pwd",
                                  "cd",   "complete", NULL};

int write_to_file(const char input[OUTPUT_CAPACITY], const char *file_path,
                  const char *mode) {
  FILE *fptr = fopen(file_path, mode);

  if (fptr == NULL) {
    return 1;
  }

  fprintf(fptr, "%s", input);

  fclose(fptr);

  return 0;
}

char check_for_redirect(char *parts[INPUT_CAPACITY], const size_t ps,
                        char redirect_file_path[PATH_MAX],
                        int *is_append_redirect) {
  for (size_t i = 0; i < ps; i++) {
    char *part = parts[i];

    if (strlen(part) && part[strlen(part) - 1] == '>') {
      if (i + 1 < ps) {
        snprintf(redirect_file_path, PATH_MAX, "%s", parts[i + 1]);
      }

      *is_append_redirect =
          strlen(part) == 3 || (strlen(part) == 2 && part[0] == '>') ? 1 : 0;

      return strlen(part) == 1 || part[0] == '>' ? '1' : part[0];
    }
  }

  return '\0';
}

void free_input_parts(char *parts[INPUT_CAPACITY], const size_t parts_size) {
  for (size_t i = 0; i < parts_size; i++) {
    char *part = parts[i];
    if (part != NULL) {
      free(part);
    }
  }
}

size_t input_parser(const char input[INPUT_CAPACITY],
                    char *parts[INPUT_CAPACITY]) {
  const size_t input_size = strlen(input);

  char token_buffer[INPUT_CAPACITY] = {0};
  size_t is_single_quoting = 0;
  size_t is_double_quoting = 0;
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

int find_complete_history_completer(char **complete_history,
                                    size_t complete_history_size,
                                    char *lookup_cmd, char **completer) {

  int is_cmd_found = 0;

  for (size_t i = 0; i < complete_history_size; i++) {
    if (complete_history[i] == NULL)
      continue;

    char *history_elem = strdup(complete_history[i]);

    char *prev_cmd = strtok(history_elem, ":");
    char *prev_path = strtok(NULL, ":");

    if (strcmp(prev_cmd, lookup_cmd) == 0) {
      *completer = strdup(prev_path);
    }

    free(history_elem);

    if (*completer)
      return 1;
  }

  return 0;
}

char *cmd_name_generator(const char *text, int state) {
  static int list_index, len, is_in_cwd;
  static struct dirent *entry;
  static char *path, *subpath;
  static const char *name;
  static DIR *dir = NULL;

  char *result = NULL;

  if (!state) {
    list_index = 0;
    len = strlen(text);

    if (dir) {
      closedir(dir);
      dir = NULL;
    }

    if (path) {
      free(path);
      path = NULL;
    }

    if (strncmp(text, "./", 2) == 0) {
      dir = opendir("./");
      is_in_cwd = 1;
    } else {
      path = strdup(getenv("PATH"));
      subpath = strtok(path, PATH_SEP);
      dir = opendir(subpath);
      is_in_cwd = 0;
    }
  }

  while ((name = cmd_names[list_index])) {
    list_index++;

    if (strncmp(name, text, len) == 0) {
      result = strdup(name);
      break;
    }
  }

  if (result != NULL)
    return result;

  if (dir != NULL) {
    entry = readdir(dir);

    char cmp_target[PATH_MAX] = {0};

    // FIX DRY
    if (entry == NULL && !is_in_cwd) {
      subpath = strtok(NULL, PATH_SEP);

      while (subpath != NULL && entry == NULL) {
        if (dir != NULL) {
          closedir(dir);
        }

        dir = opendir(subpath);

        if (dir != NULL) {
          entry = readdir(dir);
        } else {
          subpath = strtok(NULL, PATH_SEP);
        }
      }
    }
    // FIX DRY

    while (entry != NULL) {
      snprintf(cmp_target, PATH_MAX, !is_in_cwd ? "%s" : "./%s", entry->d_name);

      if (strncmp(cmp_target, text, len) == 0) {
        result = strdup(cmp_target);
        break;
      }

      entry = readdir(dir);

      if (entry == NULL && !is_in_cwd) {
        subpath = strtok(NULL, PATH_SEP);

        while (subpath != NULL && entry == NULL) {
          if (dir != NULL) {
            closedir(dir);
          }

          dir = opendir(subpath);

          if (dir != NULL) {
            entry = readdir(dir);
          } else {
            subpath = strtok(NULL, PATH_SEP);
          }
        }
      }
    };
  }

  if (result == NULL) {
    if (path) {
      free(path);
      path = NULL;
    }

    if (dir) {
      closedir(dir);
      dir = NULL;
    }
  }

  return result;
}

char *completer_generator(const char *text, int state) {
  static int list_index, len;
  char *match;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  match = curr_completer_value[list_index];

  if (match == NULL)
    return NULL;

  do {
    if (strncmp(match, text, len) == 0) {
      list_index++;
      return match;
    }
  } while ((match = curr_completer_value[++list_index]) != NULL);

  return NULL;
}

char **cmd_name_completion(const char *text, int start, int end) {
  rl_attempted_completion_over = 1;

  if (start == 0) {
    char **matches = rl_completion_matches(text, cmd_name_generator);

    return matches;
  }

  char *current_line = strdup(rl_line_buffer);
  char *first_arg = strtok(current_line, " ");
  char *second_arg = NULL;
  char *third_arg = NULL;
  char *completer = NULL;
  char completer_with_args[INPUT_CAPACITY];

  if (first_arg != NULL) {
    second_arg = strtok(NULL, " ");
  }

  if (second_arg != NULL) {
    third_arg = strtok(NULL, " ");
  }

  if (find_complete_history_completer(complete_cmd_state.complete_history,
                                      complete_cmd_state.complete_history_size,
                                      first_arg, &completer)) {

    snprintf(completer_with_args, INPUT_CAPACITY,
             "bash -c \"COMP_LINE='%s' COMP_POINT=%ld %s %s %s %s\"",
             rl_line_buffer, strlen(rl_line_buffer), completer,
             first_arg == NULL ? "" : first_arg, text,
             third_arg == NULL ? "" : second_arg);

    FILE *completer_stdout = popen(completer_with_args, "r");

    size_t line_count = 0;
    char line[OUTPUT_CAPACITY];
    size_t index = 0;
    int ch;

    if (completer_stdout != NULL) {
      printf("DEBUG: Executing command:\n");
      printf("[%s]\n", completer_with_args);

      while ((ch = fgetc(completer_stdout)) != EOF) {
        if (ch == '\n')
          line_count++;
      }

      int status = pclose(completer_stdout);
      if (status == -1) {
        perror("pclose failed");
      } else {
        // Use macros from <sys/wait.h> to interpret the status
        if (WIFEXITED(status)) {
          printf("Child exited with code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
          printf("Child was killed by signal: %d\n", WTERMSIG(status));
        }
      }
    }

    completer_stdout = popen(completer_with_args, "r");

    if (completer_stdout != NULL && line_count > 0) {
      curr_completer_value = calloc(line_count + 1, sizeof(char *));

      while (fgets(line, sizeof(line), completer_stdout) != NULL &&
             index < line_count) {
        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
          line[len - 1] = '\0';
        }

        curr_completer_value[index++] = strdup(line);
      }

      pclose(completer_stdout);
    } else {
      curr_completer_value = calloc(1, sizeof(char *));
    }

    char **matches = rl_completion_matches(text, completer_generator);

    for (size_t i = 0; i < line_count; i++) {
      char *line = curr_completer_value[i];
      if (line != NULL) {
        free(line);
        curr_completer_value[i] = NULL;
      }
    }

    free(completer);

    return matches;
  } else {
    char **file_matches =
        rl_completion_matches(text, rl_filename_completion_function);

    return file_matches;
  }

  free(current_line);

  return NULL;
}

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  char *line;

  rl_attempted_completion_function = cmd_name_completion;

  while ((line = readline("$ ")) != NULL) {
    char input[INPUT_CAPACITY] = {0};
    char stdout_value[OUTPUT_CAPACITY] = {0};
    char stderr_value[OUTPUT_CAPACITY] = {0};
    char *parts[INPUT_CAPACITY] = {0};

    // get user input
    snprintf(input, INPUT_CAPACITY, "%s", line);

    // check if not empty
    if (strlen(input) > 0) {
      // split it by space and store the parts in parts
      size_t parts_size = input_parser(input, parts);
      char redirect_file_path[PATH_MAX];

      // check if there a redirect >
      int is_append_redirect = 0;
      char redirect_type = check_for_redirect(
          parts, parts_size, redirect_file_path, &is_append_redirect);

      // modify the parts
      if (redirect_type != '\0') {
        size_t start = 0;
        size_t ps = parts_size;
        while (start < parts_size &&
               (strlen(parts[start]) == 0 ||
                parts[start][strlen(parts[start]) - 1] != '>')) {
          start++;
        }

        for (size_t i = start; i < ps; i++) {
          parts_size--;
          if (parts[i] != NULL) {
            free(parts[i]);
            parts[i] = NULL;
          }
        }
      }

      // if the input is exit exit the loop
      if (strcmp(input, "exit") == 0) {
        free_input_parts(parts, parts_size);
        break;
        // check if the first part is echo cmd
      } else if (strcmp(parts[0], "echo") == 0) {
        size_t i = 1;
        size_t stdout_value_len = strlen(stdout_value);

        // print all the sentences after the echo cmd
        while (i < parts_size) {
          snprintf(stdout_value + stdout_value_len,
                   sizeof(stdout_value) - stdout_value_len, "%s ", parts[i++]);

          stdout_value_len = strlen(stdout_value);
        }

        snprintf(stdout_value + stdout_value_len,
                 sizeof(stdout_value) - stdout_value_len, "\n");

        // check if it's a type cmd
      } else if (strcmp(parts[0], "type") == 0) {
        size_t i = 1;

        // loop over the cmds after it.
        while (i < parts_size) {
          char *cmd = parts[i++];
          size_t stdout_value_len = strlen(stdout_value);

          // check if the cmd is builtin.
          if (strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 ||
              strcmp(cmd, "type") == 0 || strcmp(cmd, "pwd") == 0 ||
              strcmp(cmd, "complete") == 0) {

            snprintf(stdout_value + stdout_value_len,
                     sizeof(stdout_value) - stdout_value_len,
                     "%s is a shell builtin\n", cmd);

            // chearch for the cmd in the PATH.
          } else {
            char full_path[PATH_MAX];

            if (lookup_program(cmd, full_path)) {
              snprintf(stdout_value + stdout_value_len,
                       sizeof(stdout_value) - stdout_value_len, "%s is %s\n",
                       cmd, full_path);
            } else {
              snprintf(stdout_value + stdout_value_len,
                       sizeof(stdout_value) - stdout_value_len,
                       "%s: not found\n", cmd);
            }
          }
        }
      } else if (strcmp(parts[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        size_t stdout_value_len = strlen(stdout_value);

        if (get_cwd(cwd, sizeof(cwd)) != NULL) {
          snprintf(stdout_value + stdout_value_len,
                   sizeof(stdout_value) - stdout_value_len, "%s\n", cwd);
        } else {
          free_input_parts(parts, parts_size);
          return 1;
        }
      } else if (strcmp(parts[0], "cd") == 0) {
        // set the path to the home if the user prvoide not second argument to
        // the cd or he uses ~
        char *path = strdup(parts[1] == NULL || strcmp(parts[1], "~") == 0
                                ? getenv("HOME")
                                : parts[1]);
        size_t stderr_value_len = strlen(stderr_value);

        // check if the path is exist.
        if (chdir(path) == -1 && errno == ENOENT) {
          snprintf(stderr_value + stderr_value_len,
                   sizeof(stderr_value) - stderr_value_len,
                   "cd: %s: No such file or directory\n", path);
        }

        free(path);
      } else if (strcmp(parts[0], "complete") == 0) {
        if (parts_size >= 3) {
          char *flag = parts[1];

          if (strcmp(flag, "-p") == 0) {
            char *lookup_cmd = parts[2];
            size_t stderr_value_len = strlen(stderr_value);
            size_t stdout_value_len = strlen(stdout_value);
            char *completer = NULL;

            if (find_complete_history_completer(
                    complete_cmd_state.complete_history,
                    complete_cmd_state.complete_history_size, lookup_cmd,
                    &completer)) {
              snprintf(stdout_value + stdout_value_len,
                       sizeof(stdout_value) - stdout_value_len,
                       "complete -C '%s' %s", completer, lookup_cmd);

              free(completer);
            } else {
              snprintf(stderr_value + stderr_value_len,
                       sizeof(stderr_value) - stderr_value_len,
                       "complete: %s: no completion specification", lookup_cmd);
            }
          } else if (parts_size > 3 && strcmp(flag, "-C") == 0) {
            if (complete_cmd_state.complete_history_size <
                COMPLETE_HISTORY_CAPACITY) {
              char *new_cmd = parts[3];
              char *new_path = parts[2];
              char cmd_to_path[COMPLETE_HISTORY_ELEM_CAPACITY] = {0};

              if (strlen(new_cmd) + strlen(new_path) + 2 <
                  COMPLETE_HISTORY_ELEM_CAPACITY) {
                snprintf(cmd_to_path, COMPLETE_HISTORY_ELEM_CAPACITY, "%s:%s",
                         new_cmd, new_path);
                int is_new_cmd = 1;

                for (size_t i = 0; i < complete_cmd_state.complete_history_size;
                     i++) {

                  if (complete_cmd_state.complete_history[i] == NULL)

                    continue;

                  char *history_elem =
                      strdup(complete_cmd_state.complete_history[i]);

                  char *prev_cmd = strtok(history_elem, ":");

                  if (strcmp(prev_cmd, new_cmd) == 0) {
                    is_new_cmd = 0;
                    free(complete_cmd_state.complete_history[i]);
                    complete_cmd_state.complete_history[i] =
                        strdup(cmd_to_path);
                  }

                  free(history_elem);

                  if (!is_new_cmd)
                    break;
                }

                if (is_new_cmd) {

                  complete_cmd_state.complete_history
                      [complete_cmd_state.complete_history_size++] =

                      strdup(cmd_to_path);
                }
              }
            }
          }
        }
      } else {
        char full_path[PATH_MAX] = {0};

        if (lookup_program(parts[0], full_path)) {
          int stdout_pipe[2];
          int stderr_pipe[2];
          // make the pipe before the fork so the child inherit it.
          // the pipe works this way, anything get written to the fds[1] you can
          // read it from fds[0]
          pipe(stdout_pipe);
          pipe(stderr_pipe);

          pid_t pid = fork();

          // child
          if (pid == 0) {
            // close the read end because child only write
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            // make whatever the fd=1 "stdout" point from terminal to whatever
            // fds[1] points to, this is how we get the the stdout_value from
            // the terminal
            dup2(stdout_pipe[1], 1);
            dup2(stderr_pipe[1], 2);
            // close the write end because the child does not need to write
            // anymore
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);
            // exucute the command
            execvp(full_path, parts);
            // this happend if the execvp fails
            perror("execvp");
            exit(1);
            // parent
          } else {
            // close the write end in the parent because the parent only read.
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            // get the data that the fds[0] points
            FILE *f_out = fdopen(stdout_pipe[0], "r");
            FILE *f_err = fdopen(stderr_pipe[0], "r");
            char line[256];
            size_t stdout_value_len = strlen(stdout_value);
            size_t stderr_value_len = strlen(stderr_value);

            // read it
            while (fgets(line, sizeof(line), f_out) != NULL) {
              snprintf(stdout_value + stdout_value_len,
                       sizeof(stdout_value) - stdout_value_len, "%s", line);
              stdout_value_len = strlen(stdout_value);
            }

            while (fgets(line, sizeof(line), f_err) != NULL) {
              snprintf(stderr_value + stderr_value_len,
                       sizeof(stderr_value) - stderr_value_len, "%s", line);
              stderr_value_len = strlen(stderr_value);
            }

            wait(NULL);
          }
        } else {
          size_t stderr_value_len = strlen(stderr_value);
          snprintf(stderr_value + stderr_value_len,
                   sizeof(stderr_value) - stderr_value_len,
                   "%s: command not found\n", input);
        }
      }

      char *terminal_output =
          strlen(stderr_value) ? stderr_value : stdout_value;

      if (redirect_type == '1') {
        if (write_to_file(stdout_value, redirect_file_path,
                          is_append_redirect ? "a+" : "w+")) {
          free_input_parts(parts, parts_size);
          return 1;
        }

        terminal_output = stderr_value;

      } else if (redirect_type == '2') {
        if (write_to_file(stderr_value, redirect_file_path,
                          is_append_redirect ? "a+" : "w+")) {
          free_input_parts(parts, parts_size);
          return 1;
        }

        terminal_output = stdout_value;
      }

      if (strlen(terminal_output)) {
        printf("%s", terminal_output);

        if (terminal_output[strlen(terminal_output) - 1] != '\n') {
          printf("\n");
        }
      }

      free_input_parts(parts, parts_size);
    }
  }

  free(line);

  for (size_t i = 0; i < complete_cmd_state.complete_history_size; i++) {
    char *history_elem = complete_cmd_state.complete_history[i];

    if (history_elem == NULL)
      continue;

    free(history_elem);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////
// COMP1521 21t2 -- Assignment 2 -- shuck, A Simple Shell
// <https://www.cse.unsw.edu.au/~cs1521/21T2/assignments/ass2/index.html>
//
// Written by Hugo Giles (z5309502) on 2/8/21 - 5/8/21.
//
// 2021-07-12    v1.0    Team COMP1521 <cs1521@cse.unsw.edu.au>
// 2021-07-21    v1.1    Team COMP1521 <cs1521@cse.unsw.edu.au>
//     * Adjust qualifiers and attributes in provided code,
//       to make `dcc -Werror' happy.
//

#include <sys/types.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// [[ TODO: put any extra `#include's here ]]
#include <limits.h>
#include <spawn.h>
#include <ctype.h>
#include <glob.h>


//
// Interactive prompt:
//     The default prompt displayed in `interactive' mode --- when both
//     standard input and standard output are connected to a TTY device.
//
static const char *const INTERACTIVE_PROMPT = "shuck& ";

//
// Default path:
//     If no `$PATH' variable is set in Shuck's environment, we fall
//     back to these directories as the `$PATH'.
//
static const char *const DEFAULT_PATH = "/bin:/usr/bin";

//
// Default history shown:
//     The number of history items shown by default; overridden by the
//     first argument to the `history' builtin command.
//     Remove the `unused' marker once you have implemented history.
//
static const int DEFAULT_HISTORY_SHOWN __attribute__((unused)) = 10;

//
// Input line length:
//     The length of the longest line of input we can read.
//
static const size_t MAX_LINE_CHARS = 1024;

//
// Special characters:
//     Characters that `tokenize' will return as words by themselves.
//
static const char *const SPECIAL_CHARS = "!><|";

//
// Word separators:
//     Characters that `tokenize' will use to delimit words.
//
static const char *const WORD_SEPARATORS = " \t\r\n";

// [[ TODO: put any extra constants here ]]


// [[ TODO: put any type definitions (i.e., `typedef', `struct', etc.) here ]]


static void execute_command(char **words, char **path, char **environment);
static void do_exit(char **words);
static int is_executable(char *pathname);
static char **tokenize(char *s, char *separators, char *special_chars);
static void free_tokens(char **tokens);

int pwd_call(char **words, char **path, char **environment);
int cd_call(char **words, char **path, char **environment);
int history_call(char **words, char **path, char **environment);
int exclamation_call(char **words, char **path, char **environment);
int running_command_call(char **words, char **path, char **environment,
    char *program, int stdin_copy, int stdout_copy);
char *find_path(char *program, char **path);

static char **setup_less(char *s, char *separators, char *special_chars);
static char **setup_le(char *s, char *separators, char *special_chars);

static void add_to_history(char **words);
int lines_in_history();
int is_number(char *string);
int string_in_list(char **list_of_strings, char *string);


int main (void)
{
    // Ensure `stdout' is line-buffered for autotesting.
    setlinebuf(stdout);

    // Environment variables are pointed to by `environ', an array of
    // strings terminated by a NULL value -- something like:
    //     { "VAR1=value", "VAR2=value", NULL }
    extern char **environ;

    // Grab the `PATH' environment variable for our path.
    // If it isn't set, use the default path defined above.
    char *pathp;
    if ((pathp = getenv("PATH")) == NULL) {
        pathp = (char *) DEFAULT_PATH;
    }
    char **path = tokenize(pathp, ":", "");

    // Should this shell be interactive?
    bool interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);

    // Main loop: print prompt, read line, execute command
    while (1) {
        // If `stdout' is a terminal (i.e., we're an interactive shell),
        // print a prompt before reading a line of input.
        if (interactive) {
            fputs(INTERACTIVE_PROMPT, stdout);
            fflush(stdout);
        }

        char line[MAX_LINE_CHARS];
        if (fgets(line, MAX_LINE_CHARS, stdin) == NULL)
            break;

        // Tokenise and execute the input line.
        char **command_words =
            tokenize(line, (char *) WORD_SEPARATORS, (char *) SPECIAL_CHARS);
        execute_command(command_words, path, environ);
        free_tokens(command_words);
    }

    free_tokens(path);
    return 0;
}


//
// Execute a command, and wait until it finishes.
//
//  * `words': a NULL-terminated array of words from the input command line
//  * `path': a NULL-terminated array of directories to search in;
//  * `environment': a NULL-terminated array of environment variables.
//
static void execute_command(char **words, char **path, char **environment)
{
    assert(words != NULL);
    assert(path != NULL);
    assert(environment != NULL);

    char *program = words[0];

    if (program == NULL) {
        // nothing to do
        return;
    }

    if (strcmp(program, "exit") == 0) {
        do_exit(words);
        // `do_exit' will only return if there was an error.
        return;
    }

    // Make a copy for stdin and stdout so we can easily switch back our
    // redirections in the future
    int stdout_copy = dup(STDOUT_FILENO);
    int stdin_copy = dup(STDIN_FILENO);



    // Filename expansion (Globbing), create a new array called 'glob_words',
    // an adjusted version of 'words'. We still use the original 'words' to add
    // to history later
    char **glob_words = (char**)malloc(100 * sizeof *glob_words);

    int i = 0;
    int j = 0;
    while (words[i] != NULL) {
        if ((strchr(words[i], '*') != NULL) ||
        (strchr(words[i], '?') != NULL) || (strchr(words[i], '[') != NULL) ||
        (strchr(words[i], '~') != NULL)) {
            glob_t matches;
            glob(words[i], GLOB_NOCHECK|GLOB_TILDE, NULL, &matches);
            if ((int)matches.gl_pathc == 0) {
                glob_words[j] = (char*)malloc(50 * sizeof(char));
                strcpy(glob_words[j], words[i]);
                j++;
            }
            else {
                int p = 0;
                while (p < (int)matches.gl_pathc) {
                    glob_words[j + p] = (char*)malloc(50 * sizeof(char));
                    strcpy(glob_words[j + p], matches.gl_pathv[p]);
                    p++;
                }
                j = j + p;
            }
        }
        else {
            glob_words[j] = (char*)malloc(50 * sizeof(char));
            strcpy(glob_words[j], words[i]);
            j++;
        }
        i++;
    }
    i = j;
    while (i < 100) {
        glob_words[i] = NULL;
        i++;
    }


    // Implementation of subset 4, we use 'action_words' which is a version of
    // 'glob_words' that we can modify as we go along.
    int total_words = 0;
    while (glob_words[total_words] != NULL) {
        total_words++;
    }
    int process_status = 0;

    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        perror("posix_spawn_file_actions_init");
        return;
    }

    char **action_words = (char**)malloc(100 * sizeof *action_words);
    i = 0;
    while (glob_words[i] != NULL) {
        action_words[i] = (char*)malloc(50 * sizeof(char));
        action_words[i] = glob_words[i];
        i++;
    }

    while (i < 100) {
        action_words[i] = NULL;
        i++;
    }
    // Implementation of our redirections that don't have pipelines
    if (((string_in_list(action_words, "<") == 1) ||
        (string_in_list(action_words, ">") != 0)) &&
        (string_in_list(action_words, "|") == 0)) {

        // Implementation of our input redirection (if "<" is used)
        if ((strcmp(action_words[0], "<") == 0)) {
            if (access(action_words[1], R_OK) == -1) {
                fprintf(stderr, "%s: No such file or directory\n",
                    action_words[1]);
                return;
            }
            process_status = 1;

            program = action_words[2];

            int file = open(action_words[1], O_RDONLY, 0666);

            if (posix_spawn_file_actions_adddup2(&actions,
                file, STDIN_FILENO) != 0) {
                perror("posix_spawn_file_actions_adddup2");
                return;
            }

            if ((strcmp(program, "cd") == 0) || (strcmp(program, "pwd") == 0) ||
                (strcmp(program, "history") == 0) ||
                (strcmp(program, "!") == 0)) {
                fprintf(stderr,
                    "%s: I/O redirection not permitted for builtin commands\n",
                    program);
                return;
            }

            int index = 0;
            while (action_words[index + 2] != NULL) {
                action_words[index] = (char*)malloc(50 * sizeof(char));
                action_words[index] = action_words[index + 2];
                index ++;
            }

            while (index < 100) {
                action_words[index] = NULL;
                index ++;
            }
        }

        total_words = 0;
        while (action_words[total_words] != NULL) {
            total_words++;
        }

        // Implementation of our output redirection (if ">>" is used)
        if (total_words >= 4) {
            if ((strcmp(action_words[total_words - 3], ">") == 0) &&
                (strcmp(action_words[total_words - 2], ">") == 0)) {
                if (access(action_words[total_words - 1], F_OK) == 0) {
                    if (access(action_words[total_words - 1], W_OK) == -1) {
                        fprintf(stderr, "%s: Not writable\n", action_words[1]);
                        return;
                    }
                }
                process_status = 1;

                // prepare_less_process()
                int file = open(action_words[total_words - 1],
                    O_WRONLY | O_APPEND | O_CREAT, 0666);

                action_words[total_words - 3] = NULL;
                action_words[total_words - 2] = NULL;
                action_words[total_words - 1] = NULL;

                if (posix_spawn_file_actions_adddup2(&actions,
                    file, STDOUT_FILENO) != 0) {
                    perror("posix_spawn_file_actions_adddup2");
                    return;
                }

                if ((strcmp(program, "cd") == 0) ||
                    (strcmp(program, "pwd") == 0) ||
                    (strcmp(program, "history") == 0) ||
                    (strcmp(program, "!") == 0)) {
                    fprintf(stderr,
                        "%s: I/O redirection not permitted for builtin commands\n",
                        program);
                    return;
                }
            }
        }

        total_words = 0;
        while (action_words[total_words] != NULL) {
            total_words++;
        }


        // Implementation of our output redirection (if ">" is used)
        if (total_words >= 3) {
            if ((strcmp(action_words[total_words - 2], ">") == 0)) {
                if (access(action_words[total_words - 1], F_OK) == 0) {
                    if (access(action_words[total_words - 1], W_OK) == -1) {
                        fprintf(stderr, "%s: Not writable\n", action_words[1]);
                        return;
                    }
                }

                process_status = 1;
                int file = open(action_words[total_words - 1],
                    O_WRONLY | O_CREAT | O_TRUNC, 0666);

                if (posix_spawn_file_actions_adddup2(&actions, file,
                    STDOUT_FILENO) != 0) {
                    perror("posix_spawn_file_actions_adddup2");
                    return;
                }

                total_words = 0;
                while (action_words[total_words] != NULL) {
                    total_words++;
                }

                action_words[total_words - 2] = NULL;
                action_words[total_words - 1] = NULL;

                if ((strcmp(program, "cd") == 0) ||
                    (strcmp(program, "pwd") == 0) ||
                    (strcmp(program, "history") == 0) ||
                    (strcmp(program, "!") == 0)) {
                    fprintf(stderr,
                        "%s: I/O redirection not permitted for builtin commands\n",
                        program);
                    return;
                }
            }
        }

        // If one or more redirection has taken place we create our process here
        if (process_status == 1) {
            program = action_words[0];
            char *temp_string = find_path(program, path);

            pid_t pid;
            extern char **environ;
            if (posix_spawn(&pid, temp_string, &actions, NULL, action_words,
                environ) != 0) {
                fprintf(stderr, "%s: command not found\n", program);
                return;
            }
            int exit_status;
            if (waitpid(pid, &exit_status, 0) == -1) {
                perror("waitpid");
                exit(1);
            }
            if (WIFEXITED(exit_status) == 1) {
                exit_status = WEXITSTATUS(exit_status);
            }
            dup2(stdout_copy, STDOUT_FILENO);
            close(stdout_copy);

            dup2(stdin_copy, STDIN_FILENO);
            close(stdin_copy);

            printf("%s exit status = %d\n", temp_string, exit_status);
            free(temp_string);
            add_to_history(words);
            return;
        }
    }

    // Subset 5: Pipes
    // This deals with any input that requires a pipe ("|")
    int pipes = string_in_list(action_words, "|");
    if (pipes >= 1) {
        int start_status = 0;
        int end_status = 0;
        char *filename_start = "placeholder";
        char *filename_end = "placeholder";

        total_words = 0;
        while (action_words[total_words] != NULL) {
            total_words++;
        }

        // Check if an input redirection is needed
        if ((strcmp(action_words[0], "<") == 0)) {
            if (access(action_words[1], R_OK) == -1) {
                fprintf(stderr, "%s: No such file or directory\n",
                    action_words[1]);
                return;
            }
            start_status = 1;
            filename_start = action_words[1];

            int index = 0;
            while (action_words[index + 2] != NULL) {
                action_words[index] = (char*)malloc(50 * sizeof(char));
                action_words[index] = action_words[index + 2];
                index ++;
            }

            while (index < 100) {
                action_words[index] = NULL;
                index ++;
            }
        
            total_words = 0;
            while (action_words[total_words] != NULL) {
                total_words++;
            }
        }

        // Check if an output redirection is needed (">>")
        if (total_words >= 4) {
            if ((strcmp(action_words[total_words - 3], ">") == 0) &&
                (strcmp(action_words[total_words - 2], ">") == 0)) {
                if (access(action_words[total_words - 1], F_OK) == 0) {
                    if (access(action_words[total_words - 1], W_OK) == -1) {
                        fprintf(stderr, "%s: Not writable\n", action_words[1]);
                        return;
                    }
                }
                end_status = 2;

                filename_end = action_words[total_words - 1];

                action_words[total_words - 3] = NULL;
                action_words[total_words - 2] = NULL;
                action_words[total_words - 1] = NULL;
            }
        }

        total_words = 0;
        while (action_words[total_words] != NULL) {
            total_words++;
        }

        // Check if an output redirection is needed (">")
        if (total_words >= 3) {
            if ((strcmp(action_words[total_words - 2], ">") == 0)) {
                if (access(action_words[total_words - 1], F_OK) == 0) {
                    if (access(action_words[total_words - 1], W_OK) == -1) {
                        fprintf(stderr, "%s: Not writable\n", action_words[1]);
                        return;
                    }
                }
                end_status = 1;

                filename_end = action_words[total_words - 1];

                action_words[total_words - 2] = NULL;
                action_words[total_words - 1] = NULL;
            }
        }

        // Create (pipes + 1) functions, storing each function in an array with
        // it's variables. The functions are then stored in an array together, 
        // effectively creating a 2d array of strings (3d of characters).
        char ***functions = (char***)malloc((pipes + 2) * sizeof(char**));

        int index = 0;
        int index_of_action = 0;
        while (action_words[index_of_action] != NULL) {
            functions[index] = (char**)malloc(100 * sizeof(char*));
            int index2 = 0;
            while (action_words[index_of_action] != NULL) {
                if (strcmp(action_words[index_of_action], "|") == 0) {
                    break;
                }
                functions[index][index2] =
                    (char*)malloc((strlen(action_words[index_of_action]) + 1) *
                    sizeof(char));
                strcpy(functions[index][index2], action_words[index_of_action]);
                index_of_action ++;
                index2 ++;
            }
            while (index2 < 100) {
                functions[index][index2] = NULL;
                index2 ++;
            }
            index_of_action ++;
            index ++;
        }
        functions[index] = (char**)malloc(100 * sizeof(char*));
        functions[index] = NULL;


        // Here we implement a loop which recursively goes through our functions
        // array looping output to the input of the next function.

        int pipe_file_descriptors[2];
        pid_t pid;
        int file_descriptor_in = 0;
        int exit_status;
        char *temp_string = "placeholder";
        i = 0;

        while (functions[i] != NULL) {
            if (pipe(pipe_file_descriptors) == -1) {
                perror("pipe");
                return;
            }

            posix_spawn_file_actions_t actions_pipe;
            if (posix_spawn_file_actions_init(&actions_pipe) != 0) {
                perror("posix_spawn_file_actions_init");
                return;
            }

            // If an input redirection has occured we redirect stdin to the 
            // required file. This only occurs on the first function in our 
            // pipeline.
            if (start_status == 1) {
                start_status = 0;
                int file = open(filename_start, O_RDONLY, 0666);

                if (posix_spawn_file_actions_adddup2(&actions_pipe, file,
                    STDIN_FILENO) != 0) {
                    perror("posix_spawn_file_actions_adddup2");
                    return;
                }
            }


            if (posix_spawn_file_actions_adddup2(&actions_pipe,
                file_descriptor_in, 0) != 0) {
                perror("posix_spawn_file_actions_adddup2");
                return;
            }
            if (functions[i + 1] != NULL) {
                if (posix_spawn_file_actions_adddup2(&actions_pipe,
                    pipe_file_descriptors[1], 1) != 0) {
                    perror("posix_spawn_file_actions_adddup2");
                    return;
                }
            }
            else {
                if (posix_spawn_file_actions_adddup2(&actions_pipe,
                    stdout_copy, STDOUT_FILENO) != 0) {
                    perror("posix_spawn_file_actions_adddup2");
                    return;
                }

                // If an output redirection has occured we redirect stdin to the 
                // required file. This only occurs on the last function in our 
                // pipeline. 'end_status' = 2, represents a ">>" redirection and
                // 'end_stats' = 1, represents a ">" redirection.
                if (end_status == 1) {
                    end_status = 0;
                    int file = open(filename_end, O_WRONLY | O_CREAT | O_TRUNC,
                        0666);

                    if (posix_spawn_file_actions_adddup2(&actions_pipe, file,
                        STDOUT_FILENO) != 0) {
                        perror("posix_spawn_file_actions_adddup2");
                        return;
                    }
                }
                if (end_status == 2) {
                    end_status = 0;
                    int file = open(filename_end,
                        O_WRONLY | O_APPEND | O_CREAT, 0666);

                    if (posix_spawn_file_actions_adddup2(&actions, file,
                        STDOUT_FILENO) != 0) {
                        perror("posix_spawn_file_actions_adddup2");
                        return;
                    }
                }
            }
            if ((strcmp(functions[i][0], "cd") == 0) ||
                (strcmp(functions[i][0], "pwd") == 0) ||
                (strcmp(functions[i][0], "history") == 0) ||
                (strcmp(functions[i][0], "!") == 0)) {
                fprintf(stderr,
                    "%s: I/O redirection not permitted for builtin commands\n",
                    functions[i][0]);
                return;
            }

            // We now spawn the process and loop back to start the next process
            // the file descriptor where the output goes is saved as
            // 'file_descriptor_in' which is connected to the next process' 
            // input.

            if (posix_spawn_file_actions_addclose(&actions_pipe,
                pipe_file_descriptors[0]) != 0) {
                perror("posix_spawn_file_actions_init");
                return;
            }

            temp_string = find_path(functions[i][0], path);

            extern char **environ;
            if (posix_spawn(&pid, temp_string, &actions_pipe, NULL,
                functions[i], environ) != 0) {
                perror("spawn");
                return;
            }

            if (waitpid(pid, &exit_status, 0) == -1) {
                perror("waitpid");
                return;
            }

            close(pipe_file_descriptors[1]);
            file_descriptor_in = pipe_file_descriptors[0];
            i++;
        }
        printf("%s exit status = %d\n", temp_string, exit_status);
        add_to_history(words);
        return;
    }

    // Subset 0:
    // Implementing pwd
    if (strcmp(program, "pwd") == 0) {
        if (pwd_call(glob_words, path, environment) == 1) {
            add_to_history(words);
        }
        return;
    }

    // Implementing cd
    if (strcmp(program, "cd") == 0) {
        if (cd_call(glob_words, path, environment) == 1) {
            add_to_history(words);
        }
        return;
    }

    // Subset 2
    // history implementation
    if (strcmp(program, "history") == 0) {
        if (history_call(glob_words, path, environment) == 1) {
            add_to_history(words);
        }
        return;
    }

    // implementation of !
    if (strcmp(program, "!") == 0) {
        exclamation_call(glob_words, path, environment);
        return;
    }

    // Subset 1
    // Implementation of running commands
    if (running_command_call(glob_words, path, environment, program, stdin_copy,
        stdout_copy) == 1) {
        add_to_history(words);
    }
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);

    dup2(stdin_copy, STDIN_FILENO);
    close(stdin_copy);
    return;
    

    if (strrchr(program, '/') == NULL) {
        fprintf(stderr, "--- UNIMPLEMENTED: searching for a program to run\n");
    }

    if (is_executable(program)) {
        fprintf(stderr, "--- UNIMPLEMENTED: running a program\n");
    } else {
        fprintf(stderr, "--- UNIMPLEMENTED: error when we can't run anything\n");
    }
}

// Deals with a "pwd" call, returns 1 on success and 0 on failure
int pwd_call(char **words, char **path, char **environment) {
    char pathname[PATH_MAX];
    if (getcwd(pathname, sizeof pathname) == NULL) {
        perror("getcwd");
        return 0;
    }
    printf("current directory is '%s'\n", pathname);
    return 1;
}

// Deals with a "cd" call, returns 1 on success and 0 on failure
int cd_call(char **words, char **path, char **environment) {
    char *destination;
    if (words[1] == NULL) {
        destination = getenv( "HOME" );
    }
    else {
        destination = words[1];
    }
    if (chdir(destination) != 0) {
        fprintf(stderr, "cd: %s: No such file or directory\n", destination);
        return 0;
    }
    return 1;
}

// Deals with a "history" call, returns 1 on success and 0 on failure
int history_call(char **words, char **path, char **environment) {
    char original_path[PATH_MAX];

    if (getcwd(original_path, sizeof original_path) == NULL) {
        perror("getcwd");
        return 0;
    }

    char *home = getenv( "HOME" );
    if (chdir(home) != 0) {
        perror("chdir");
        return 0;
    }

    FILE *file = fopen(".shuck_history", "r");

    if (words[1] != NULL)  {
        if (is_number(words[1]) == 0) {
            fprintf(stderr, "%s: %s: numeric argument required\n",
            words[0], words[1]);
            return 0;
        }
        else if (words[2] != NULL) {
            fprintf(stderr, "%s: too many arguments\n", words[0]);
            return 0;
        }
    }

    int number_of_lines = 10;
    if (words[1] != NULL) {
        number_of_lines = atoi(words[1]);
    }


    int count = 0;
    int starting_row = lines_in_history() - number_of_lines;
    if (starting_row < 0) {
        starting_row = 0;
    }

    char line_to_print[MAX_LINE_CHARS];
    while (fgets(line_to_print, sizeof line_to_print, file) != NULL) {
        if (count >= starting_row) {
            printf("%d: %s", count, line_to_print);
            count ++;
        }
        else {
            count ++;
        }
    }

    fclose(file);
    if (chdir(original_path) != 0) {
        perror("chdir");
        return 0;
    }
    return 1;
}

// Deals with a "!" call, returns 1 on success and 0 on failure
int exclamation_call(char **words, char **path, char **environment) {
    int n;
    if (words[1] == NULL) {
        n = lines_in_history() - 1;
    }
    else {
        n = atoi(words[1]);
    }

    char original_path[PATH_MAX];

    if (getcwd(original_path, sizeof original_path) == NULL) {
        perror("getcwd");
        return 0;
    }

    char *home = getenv( "HOME" );
    if (chdir(home) != 0) {
        perror("chdir");
        return 0;
    }

    FILE *file = fopen(".shuck_history", "r");

    int count = 0;
    char line_to_use[MAX_LINE_CHARS];
    while (fgets(line_to_use, sizeof line_to_use, file) != NULL) {
        if (count == n) {
            break;
        }
        else {
            count++;
        }
    }
    fclose(file);

    if (chdir(original_path) != 0) {
        perror("chdir");
        return 0;
    }

    line_to_use[strcspn(line_to_use, "\n")] = 0;
    printf("%s\n", line_to_use);
    char **new_words = tokenize(line_to_use, " ", "");
    execute_command(new_words, path, environment);
    return 1;
}

// Deals with a running command call, returns 1 on success and 0 on failure
int running_command_call(char **words, char **path, char **environment,
    char *program, int stdin_copy, int stdout_copy) {
    
    char *temp_string = find_path(program, path);

    pid_t pid;
    extern char **environ;
    if (posix_spawn(&pid, temp_string, NULL, NULL, words, environ) != 0) {
        fprintf(stderr, "%s: command not found\n", program);
        return 0;
    }
    int exit_status;
    if (waitpid(pid, &exit_status, 0) == -1) {
        perror("waitpid");
        exit(1);
    }
    if (WIFEXITED(exit_status) == 1) {
        exit_status = WEXITSTATUS(exit_status);
    }
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);

    dup2(stdin_copy, STDIN_FILENO);
    close(stdin_copy);

    printf("%s exit status = %d\n", temp_string, exit_status);
    free(temp_string);
    return 1;
}

// Finds the path of a command and returns it as a string
char *find_path(char *program, char **path) {
    int i = 0;
    char *temp_string;
    if (strchr(program, '/') != NULL) {
        temp_string = malloc(strlen(program) + 1);
        strcpy(temp_string, program);
    }
    else {
        temp_string = malloc(strlen(program) + strlen(path[i]) + 2);
        while (path[i] != NULL) {
            temp_string = malloc(strlen(program) + strlen(path[i]) + 2);
            strcpy(temp_string, path[i]);
            strcat(temp_string, "/");
            strcat(temp_string, program);
            if (is_executable(temp_string) == 1) {
                break;
            }
            i++;
        }
    }
    return temp_string;
}

// Adds words to history
static void add_to_history(char **words) {
    // Save command to history folder
    char original_path[PATH_MAX];

    if (getcwd(original_path, sizeof original_path) == NULL) {
        perror("getcwd");
        return;
    }

    char *home = getenv( "HOME" );
    if (chdir(home) != 0) {
        perror("chdir");
        return;
    }

    FILE *file = fopen(".shuck_history", "a");

    int h = 0;
    while (words[h] != NULL) {
        fprintf(file, "%s ", words[h]);
        h++;
    }
    fprintf(file, "\n");
    fclose(file);
    if (chdir(original_path) != 0) {
        perror("chdir");
        return;
    }
}

// Counts the number of lines in history
int lines_in_history() {
    char original_path[PATH_MAX];
    if (getcwd(original_path, sizeof original_path) == NULL) {
        perror("getcwd");
    }
    char *home = getenv( "HOME" );
    if (chdir(home) != 0) {
        perror("chdir");
    }
    FILE *file = fopen(".shuck_history", "r");

    int current;
    int number_of_lines = 0;
    for (current = getc(file); current != EOF; current = getc(file)) {
        if (current == '\n') {
            number_of_lines ++;
        }
    }

    fclose(file);
    if (chdir(original_path) != 0) {
        perror("chdir");
    }
    return number_of_lines;
}

// Returns 1 if a string is a number or 0 if it is not
int is_number(char *string) {
    for (int i = 0; string[i] != '\0'; i++) {
        if (isdigit(string[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

// Returns the number of the target string in a list of strings, used for
// finding how many pipes in a pipeline call
int string_in_list(char **list_of_strings, char *string) {
    int i = 0;
    int status = 0;
    while (list_of_strings[i] != NULL) {
        if (strcmp(list_of_strings[i], string) == 0) {
            status ++;
        }
        i++;
    }
    return status;
}




//
// Implement the `exit' shell built-in, which exits the shell.
//
// Synopsis: exit [exit-status]
// Examples:
//     % exit
//     % exit 1
//
static void do_exit(char **words)
{
    assert(words != NULL);
    assert(strcmp(words[0], "exit") == 0);

    int exit_status = 0;

    if (words[1] != NULL && words[2] != NULL) {
        // { "exit", "word", "word", ... }
        fprintf(stderr, "exit: too many arguments\n");

    } else if (words[1] != NULL) {
        // { "exit", something, NULL }
        char *endptr;
        exit_status = (int) strtol(words[1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "exit: %s: numeric argument required\n", words[1]);
        }
    }

    exit(exit_status);
}


//
// Check whether this process can execute a file.  This function will be
// useful while searching through the list of directories in the path to
// find an executable file.
//
static int is_executable(char *pathname)
{
    struct stat s;
    return
        // does the file exist?
        stat(pathname, &s) == 0 &&
        // is the file a regular file?
        S_ISREG(s.st_mode) &&
        // can we execute it?
        faccessat(AT_FDCWD, pathname, X_OK, AT_EACCESS) == 0;
}


//
// Split a string 's' into pieces by any one of a set of separators.
//
// Returns an array of strings, with the last element being `NULL'.
// The array itself, and the strings, are allocated with `malloc(3)';
// the provided `free_token' function can deallocate this.
//
static char **tokenize(char *s, char *separators, char *special_chars)
{
    size_t n_tokens = 0;

    // Allocate space for tokens.  We don't know how many tokens there
    // are yet --- pessimistically assume that every single character
    // will turn into a token.  (We fix this later.)
    char **tokens = calloc((strlen(s) + 1), sizeof *tokens);
    assert(tokens != NULL);

    while (*s != '\0') {
        // We are pointing at zero or more of any of the separators.
        // Skip all leading instances of the separators.
        s += strspn(s, separators);

        // Trailing separators after the last token mean that, at this
        // point, we are looking at the end of the string, so:
        if (*s == '\0') {
            break;
        }

        // Now, `s' points at one or more characters we want to keep.
        // The number of non-separator characters is the token length.
        size_t length = strcspn(s, separators);
        size_t length_without_specials = strcspn(s, special_chars);
        if (length_without_specials == 0) {
            length_without_specials = 1;
        }
        if (length_without_specials < length) {
            length = length_without_specials;
        }

        // Allocate a copy of the token.
        char *token = strndup(s, length);
        assert(token != NULL);
        s += length;

        // Add this token.
        tokens[n_tokens] = token;
        n_tokens++;
    }

    // Add the final `NULL'.
    tokens[n_tokens] = NULL;

    // Finally, shrink our array back down to the correct size.
    tokens = realloc(tokens, (n_tokens + 1) * sizeof *tokens);
    assert(tokens != NULL);

    return tokens;
}


//
// Free an array of strings as returned by `tokenize'.
//
static void free_tokens(char **tokens)
{
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

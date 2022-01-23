if ((strcmp(functions[i][0], "cd") == 0) || (strcmp(functions[i][0], "pwd") == 0) ||
                    (strcmp(functions[i][0], "history") == 0) || (strcmp(functions[i][0], "!") == 0)) {
                    fprintf(stderr, "%s: I/O redirection not permitted for builtin commands\n", functions[i][0]);
                    return;
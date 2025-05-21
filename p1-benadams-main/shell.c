#include "tokenize.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "vect.h"
#include "string.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 255

char prev_command[1024] = "";


/**
 * Function which prints the help information
 */

void print_help() {
    printf("Built-in commands:\n");
    printf("1. cd <directory>\n");
    printf("   Change the current directory to <directory>.\n");
    printf("   Usage: cd /path/to/directory\n\n");

    printf("2. exit\n");
    printf("   Exit the shell.\n");
    printf("   Usage: exit\n\n");

    printf("3. source <script_file>\n");
    printf("   Execute commands from a file as if they were entered at the command line.\n");
    printf("   Usage: source script.txt\n\n");

    printf("4. prev\n");
    printf("   Re-execute the last command entered.\n");
    printf("   Usage: prev\n\n");

    printf("5. help\n");
    printf("   Display this help information.\n");
    printf("   Usage: help\n");
}

/**
 * determines if a character is a defined special character
 */
int is_special_character(char ch) {
  return ch == '(' || ch == ')' || ch == '<' || ch == '>' || ch == ';' || ch == '|'; 
}

/**
 * Read a quoted string and handle everything as a single token
 */
int read_quoted_string(const char *input, char *output) {
    int i = 0;
    ++input; 
    while (input[i] != '\0' && input[i] != '"') {
        // copy character to output buffer
        output[i] = input[i]; 
        ++i;
    }
    output[i] = '\0'; 

    return i + 2;
}


/**
 * Read a regular word, stop if you hit a space, special character, or the end of the inout
 */
int read_word(const char *input, char *output) {
    int i = 0;
    // While the character is not a space and not a special shell character, 
    while (input[i] != '\0' && input[i] != ' ' && !is_special_character(input[i]) && input[i] != '"') {
        // copy character to output buffer
        output[i] = input[i]; 
        ++i;
    }
    output[i] = '\0'; 

    return i; 
}

/**
 * trims off trailing newlines '\n' from a string
 */
void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0'; // Remove trailing newline
    }
}

/**
 * tokenizes a user input and returns a vector containing
 * all of the tokens
 */
vect_t* tokenize_input(const char *input) {
    vect_t *tokens = vect_new();
    char buf[256];
    int i = 0;

    while (input[i] != '\0') {
        if (input[i] == ' ') {
            ++i;
            continue;
        }
        if (input[i] == '"') {
            i += read_quoted_string(&input[i], buf);
            vect_add(tokens, buf);
            continue;
        }
        if (is_special_character(input[i])) {
            buf[0] = input[i];
            buf[1] = '\0';
            vect_add(tokens, buf);
            ++i;
            continue;
        }
        i += read_word(&input[i], buf);
        vect_add(tokens, buf);
    }
    return tokens;
}


/**
 * Excecutes a command containing a redirection argument '<', '>'
 */
void execute_redirection_command(const char *command, vect_t *args) {
    int input_redir = -1, output_redir = -1;
    int num_args = vect_size(args);
    char *argv[num_args + 1]; 

    // Parse arguments for redirection symbols
    int arg_index = 0;
    for (int i = 0; i < num_args; ++i) {
        char *arg = (char *)vect_get(args, i);
        if (strcmp(arg, "<") == 0) {
            //  Handles input redirection
            if (i + 1 < num_args) {
                input_redir = open(vect_get(args, i + 1), O_RDONLY);
                if (input_redir < 0) {
                    perror("Failed to open input file");
                    return;
                }
                i++; 
            } else {
                fprintf(stderr, "No input file specified after '<'\n");
                return;
            }
        } else if (strcmp(arg, ">") == 0) {
            // Handles output redirection
            if (i + 1 < num_args) {
                output_redir = open(vect_get(args, i + 1), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_redir < 0) {
                    perror("Failed to open output file");
                    return;
                }
                i++;
            } else {
                fprintf(stderr, "No output file specified after '>'\n");
                return;
            }
        } else {
            // Regular argument
            argv[arg_index++] = arg;
        }
    }
    argv[arg_index] = NULL; 

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (input_redir != -1) {
            dup2(input_redir, STDIN_FILENO); 
            close(input_redir); 
        }
        if (output_redir != -1) {
            dup2(output_redir, STDOUT_FILENO); 
            close(output_redir); 
        }
        execvp(command, argv);
        // If execvp fails
        fprintf(stderr, "%s: command not found\n", command);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, NULL, 0);
    } else {
        // Fork failed
        perror("Failed to fork");
    }
}

/**
 * Changes the current working directory using the chdir() method
 */
void change_directory(vect_t *args_vector) {
    if (vect_size(args_vector) < 2) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    const char *path = vect_get(args_vector, 1);
    if (chdir(path) != 0) {
        perror("cd failed");
    }
}

/**
 *  Extracts the command from a file and runs the command
 *  For the 'source' command
 */
void execute_file_script(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line); // Trim newline before processing
        if (strlen(line) == 0) continue; // Skip empty lines

        vect_t *args_vector = tokenize_input(line);

        // Trim newlines from each argument in args_vector
        for (int j = 0; j < vect_size(args_vector); j++) {
            char *arg = (char *)vect_get(args_vector, j);
            trim_newline(arg);
        }

        const char *command = vect_get(args_vector, 0);
        if (strcmp(command, "exit") == 0) {
            printf("%s\n", "Bye bye.");
            vect_delete(args_vector);
            break; 
        }

        if (strcmp(command, "cd") == 0) {
            change_directory(args_vector);
        } else {
            execute_redirection_command(command, args_vector);
        }

        vect_delete(args_vector); // Clean up
    }

    fclose(file);
}

/**
 * executes a command containing a pipe argument '|'
 * takes in the lhs and rhs of the pipe argument 
 */
void execute_piped_command(const char *lhs_command, vect_t *lhs_args, const char *rhs_command, vect_t *rhs_args) {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // Child process for the LHS command
        close(pipe_fds[0]);  
        dup2(pipe_fds[1], STDOUT_FILENO); 
        close(pipe_fds[1]);

        // Execute the LHS command
        int num_args = vect_size(lhs_args);
        char *argv[num_args + 1];
        for (int i = 0; i < num_args; ++i) {
            argv[i] = (char *)vect_get(lhs_args, i);
        }
        argv[num_args] = NULL;
        
        const char* lhscmd = vect_get(lhs_args, 0);
        execute_redirection_command(lhscmd, lhs_args);
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Child process for the right-hand side RHS command
        close(pipe_fds[1]);  
        dup2(pipe_fds[0], STDIN_FILENO);  
        close(pipe_fds[0]);

        // Execute the RHS command
        int num_args = vect_size(rhs_args);
        char *argv[num_args + 1];
        for (int i = 0; i < num_args; ++i) {
            argv[i] = (char *)vect_get(rhs_args, i);
        }
        argv[num_args] = NULL;

        const char* rhscmd = vect_get(rhs_args, 0);
        execute_redirection_command(rhscmd, rhs_args);
        exit(1);
    }

    // Parent process closes both ends of the pipe then waits for processes to finish
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/**
 * main driver for an input command
 * Passes the command to other functions which call the commands
 */
void process_command(char *input, vect_t *args_vector, char *last_command) {
    char buf[256];

    // if the user inputs "help", print help messages
    if (strcmp(input, "help") == 0) {
        print_help();
        return;
    }

    // if the user inputs "prev", change the input array to the previous stored command
    if (strcmp(input, "prev") == 0) {
        if (strlen(prev_command) > 0) {
            char *prev_sequeunce_command = strtok(prev_command, ";");
            while (prev_sequeunce_command != NULL) {
                vect_t *args_vector = vect_new();
                process_command(prev_sequeunce_command, args_vector, last_command);
                vect_delete(args_vector);

            // Get the next command after `;`
            prev_sequeunce_command = strtok(NULL, ";");
        }
        } else {
            printf("No previous command.\n");
        }
        return; 
    } 
    strcpy(last_command, input);

    // Check for the pipe '|' characeter and handle piping
    char *pipe_pos = strstr(input, "|");
    if (pipe_pos != NULL) {
        // Split input into two parts: LHS and RHS of the pipe
        *pipe_pos = '\0';  
        char *rhs_command = pipe_pos + 1;

        // Tokenize the LHS command
        vect_t *lhs_args_vector = tokenize_input(input);


        // Tokenize the RHS command
        vect_t *rhs_args_vector = tokenize_input(rhs_command);


        const char *lhs_command = vect_get(lhs_args_vector, 0);
        const char *rhs_command_name = vect_get(rhs_args_vector, 0);

        execute_piped_command(lhs_command, lhs_args_vector, rhs_command_name, rhs_args_vector);

        vect_delete(lhs_args_vector);
        vect_delete(rhs_args_vector);
        return;
    }

    // Tokenize the user's command
    args_vector = tokenize_input(input);


    // Trim newlines from each argument in args_vector
    for (int j = 0; j < vect_size(args_vector); j++) {
        char *arg = (char *)vect_get(args_vector, j);
        trim_newline(arg);
    }

    const char *command = vect_get(args_vector, 0);
    if (strcmp(command, "exit") == 0) {
        printf("%s\n", "Bye bye.");
        exit(1);
    }

    if (strcmp(command, "cd") == 0) {
        change_directory(args_vector);
    } else if (strcmp(command, "source") == 0 && vect_size(args_vector) > 1) {
        const char *script_file = vect_get(args_vector, 1);
        execute_file_script(script_file);
    } else {
        execute_redirection_command(command, args_vector);
    }
}

/**
 * main loop which handles exiting and printing 'Shell $'
 * Stores whole user input into prev command for prev call
 */
int main(int argc, char **argv) {
    printf("%s\n", "Welcome to mini-shell.");

    bool inSession = true;
    char last_command[MAX_INPUT_SIZE] = "";
    while (inSession) {
        printf("%s", "shell $ ");

        char input[MAX_INPUT_SIZE];

        // retrieve user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("%s\n", "Bye bye.");
            break;
        }

        trim_newline(input); // Trim newline from input

        if (strcmp(input, "prev") != 0) {
            strcpy(prev_command, input);
        }

        // Split input by `;` to handle multiple commands
        char *command = strtok(input, ";");
        while (command != NULL) {
            vect_t *args_vector = vect_new();
            process_command(command, args_vector, last_command);
            vect_delete(args_vector);

            // Get the next command after `;`
            command = strtok(NULL, ";");
        }
    }

    return 0;
}
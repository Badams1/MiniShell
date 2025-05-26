#include "tokenize.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "vect.h"
#include "string.h"
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define MAX_INPUT_SIZE 255
#define MAX_PATH_SIZE 1024
#define MAX_CONTENT_SIZE 4096

// Current working directory state
static char current_dir[MAX_PATH_SIZE] = "/home";
static char previous_dir[MAX_PATH_SIZE] = "/home";

// Forward declarations
extern int custom_printf(const char* format, ...);
extern int is_special_character(char ch);
extern int read_quoted_string(const char *input, char *output);
extern int read_word(const char *input, char *output);
void process_command(char *input, vect_t *args_vector);

// Virtual filesystem structure
typedef struct {
    char name[256];
    bool is_dir;
    char content[MAX_CONTENT_SIZE];
    time_t created;
    time_t modified;
} fs_entry_t;

typedef struct {
    fs_entry_t entries[100];
    int count;
} fs_dir_t;

static fs_dir_t virtual_fs = {0};

// Add the readme content as a constant
const char* README_CONTENT = "Available commands:\n\n"
    "ls      - List files in current directory\n"
    "cd      - Change directory\n"
    "pwd     - Show current directory path\n"
    "echo    - Print text to terminal\n"
    "cat     - Display file contents\n"
    "touch   - Create a new empty file\n"
    "mkdir   - Create a new directory\n"
    "rm      - Remove a file or directory\n"
    "date    - Show current date and time\n"
    "whoami  - Show current user\n"
    "clear   - Clear terminal screen\n"
    "help    - Show detailed command help\n"
    "readme  - Show this command list\n";

// Initialize virtual filesystem
void init_fs() {
    if (virtual_fs.count == 0) {
        // Add root directory
        strcpy(virtual_fs.entries[0].name, "/");
        virtual_fs.entries[0].is_dir = true;
        virtual_fs.entries[0].created = time(NULL);
        virtual_fs.entries[0].modified = time(NULL);
        
        // Add home directory
        strcpy(virtual_fs.entries[1].name, "/home");
        virtual_fs.entries[1].is_dir = true;
        virtual_fs.entries[1].created = time(NULL);
        virtual_fs.entries[1].modified = time(NULL);

        // Add README.md to home directory
        strcpy(virtual_fs.entries[2].name, "/home/README.md");
        virtual_fs.entries[2].is_dir = false;
        virtual_fs.entries[2].created = time(NULL);
        virtual_fs.entries[2].modified = time(NULL);
        strncpy(virtual_fs.entries[2].content, README_CONTENT, MAX_CONTENT_SIZE - 1);
        
        virtual_fs.count = 3;
    }
}

// Helper function to find a file/directory in virtual filesystem
fs_entry_t* find_fs_entry(const char* path) {
    for (int i = 0; i < virtual_fs.count; i++) {
        if (strcmp(virtual_fs.entries[i].name, path) == 0) {
            return &virtual_fs.entries[i];
        }
    }
    return NULL;
}

// Helper function to add a new filesystem entry
fs_entry_t* add_fs_entry(const char* path, bool is_dir) {
    if (virtual_fs.count >= 100) {
        return NULL;
    }
    
    fs_entry_t* entry = &virtual_fs.entries[virtual_fs.count++];
    strncpy(entry->name, path, 255);
    entry->is_dir = is_dir;
    entry->created = time(NULL);
    entry->modified = time(NULL);
    if (!is_dir) {
        entry->content[0] = '\0';
    }
    return entry;
}

/**
 * Function which prints the help information
 */
void print_help() {
    custom_printf("Built-in commands:\n");
    custom_printf("1. ls [-l]\n");
    custom_printf("   List files in the current directory.\n");
    custom_printf("   -l: show in long format with details\n\n");

    custom_printf("2. cd <directory>\n");
    custom_printf("   Change the current directory.\n");
    custom_printf("   Usage: cd <path> or cd - for previous directory\n\n");

    custom_printf("3. pwd\n");
    custom_printf("   Print working directory.\n");
    custom_printf("   Usage: pwd\n\n");

    custom_printf("4. echo <text>\n");
    custom_printf("   Print text to the terminal.\n");
    custom_printf("   Usage: echo Hello World\n\n");

    custom_printf("5. cat <file>\n");
    custom_printf("   Display contents of a file.\n");
    custom_printf("   Usage: cat file.txt\n\n");

    custom_printf("6. touch <file>\n");
    custom_printf("   Create a new empty file.\n");
    custom_printf("   Usage: touch file.txt\n\n");

    custom_printf("7. mkdir <directory>\n");
    custom_printf("   Create a new directory.\n");
    custom_printf("   Usage: mkdir mydir\n\n");

    custom_printf("8. rm <file/directory>\n");
    custom_printf("   Remove a file or directory.\n");
    custom_printf("   Usage: rm file.txt\n\n");

    custom_printf("9. date\n");
    custom_printf("   Display current date and time.\n");
    custom_printf("   Usage: date\n\n");

    custom_printf("10. whoami\n");
    custom_printf("    Display current user.\n");
    custom_printf("    Usage: whoami\n\n");

    custom_printf("11. clear\n");
    custom_printf("    Clear the terminal screen.\n");
    custom_printf("    Usage: clear\n\n");

    custom_printf("12. help\n");
    custom_printf("    Display this help information.\n");
    custom_printf("    Usage: help\n");
}

/**
 * trims off trailing newlines '\n' from a string
 */
void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
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
 * Implementation of basic shell commands
 */

void cmd_pwd() {
    custom_printf("%s\n", current_dir);
}

void cmd_cd(const char* path) {
    if (!path) {
        custom_printf("cd: missing argument\n");
        return;
    }

    // Handle cd - to go to previous directory
    if (strcmp(path, "-") == 0) {
        char temp[MAX_PATH_SIZE];
        strncpy(temp, current_dir, MAX_PATH_SIZE);
        strncpy(current_dir, previous_dir, MAX_PATH_SIZE);
        strncpy(previous_dir, temp, MAX_PATH_SIZE);
        return;
    }

    // Store current directory before changing
    strncpy(previous_dir, current_dir, MAX_PATH_SIZE);

    char new_path[MAX_PATH_SIZE];
    if (path[0] == '/') {
        // Absolute path
        strncpy(new_path, path, MAX_PATH_SIZE - 1);
    } else if (strcmp(path, "..") == 0) {
        // Go up one directory
        char* last_slash = strrchr(current_dir, '/');
        if (last_slash != current_dir) {
            *last_slash = '\0';
        }
        return;
    } else {
        // Relative path
        snprintf(new_path, MAX_PATH_SIZE, "%s/%s", current_dir, path);
    }

    // Check if directory exists in virtual filesystem
    fs_entry_t* entry = find_fs_entry(new_path);
    if (!entry || !entry->is_dir) {
        custom_printf("cd: %s: No such directory\n", path);
        return;
    }

    // Update current directory
    strncpy(current_dir, new_path, MAX_PATH_SIZE - 1);
}

void cmd_ls(vect_t* args) {
    bool long_format = false;
    
    // Check for -l flag
    for (int i = 1; i < vect_size(args); i++) {
        if (strcmp(vect_get(args, i), "-l") == 0) {
            long_format = true;
            break;
        }
    }

    // Show virtual filesystem entries in current directory
    if (long_format) {
        custom_printf("total %d\n", virtual_fs.count);
    }

    for (int i = 0; i < virtual_fs.count; i++) {
        fs_entry_t* entry = &virtual_fs.entries[i];
        char* entry_name = strrchr(entry->name, '/');
        if (!entry_name) {
            entry_name = entry->name;
        } else {
            entry_name++; // Skip the slash
        }

        // Only show entries in current directory
        char dir_path[MAX_PATH_SIZE];
        strncpy(dir_path, entry->name, MAX_PATH_SIZE);
        char* last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
        }
        
        if (strcmp(dir_path, current_dir) == 0) {
            if (long_format) {
                char time_str[26];
                ctime_r(&entry->modified, time_str);
                time_str[24] = '\0'; // Remove newline
                custom_printf("%s  -  guest  guest  %s  %s\n",
                    entry->is_dir ? "drwxr-xr-x" : "-rw-r--r--",
                    time_str,
                    entry_name);
            } else {
                custom_printf("%s\n", entry_name);
            }
        }
    }
}

void cmd_echo(vect_t* args) {
    for (int i = 1; i < vect_size(args); i++) {
        custom_printf("%s", (char*)vect_get(args, i));
        if (i < vect_size(args) - 1) {
            custom_printf(" ");
        }
    }
    custom_printf("\n");
}

void cmd_cat(const char* filename) {
    if (!filename) {
        custom_printf("cat: missing file operand\n");
        return;
    }

    // Special handling for readme variations
    if (strcasecmp(filename, "readme") == 0 || 
        strcasecmp(filename, "readme.md") == 0 || 
        strcasecmp(filename, "readme.txt") == 0) {
        // If in home directory, show the README.md
        if (strcmp(current_dir, "/home") == 0) {
            fs_entry_t* entry = find_fs_entry("/home/README.md");
            if (entry) {
                custom_printf("%s", entry->content);
                if (entry->content[0] != '\0') {
                    custom_printf("\n");
                }
                return;
            }
        }
    }

    char full_path[MAX_PATH_SIZE];
    if (filename[0] == '/') {
        strncpy(full_path, filename, MAX_PATH_SIZE);
    } else {
        snprintf(full_path, MAX_PATH_SIZE, "%s/%s", current_dir, filename);
    }

    fs_entry_t* entry = find_fs_entry(full_path);
    if (!entry || entry->is_dir) {
        // Try with .md extension if file not found
        if (!strstr(filename, ".")) {
            char with_ext[MAX_PATH_SIZE];
            snprintf(with_ext, MAX_PATH_SIZE, "%s.md", full_path);
            entry = find_fs_entry(with_ext);
            if (entry && !entry->is_dir) {
                custom_printf("%s", entry->content);
                if (entry->content[0] != '\0') {
                    custom_printf("\n");
                }
                return;
            }
        }
        custom_printf("cat: %s: No such file\n", filename);
        return;
    }

    custom_printf("%s", entry->content);
    if (entry->content[0] != '\0') {
        custom_printf("\n");
    }
}

void cmd_touch(const char* filename) {
    if (!filename) {
        custom_printf("touch: missing file operand\n");
        return;
    }

    char full_path[MAX_PATH_SIZE];
    if (filename[0] == '/') {
        strncpy(full_path, filename, MAX_PATH_SIZE);
    } else {
        snprintf(full_path, MAX_PATH_SIZE, "%s/%s", current_dir, filename);
    }

    fs_entry_t* entry = find_fs_entry(full_path);
    if (entry) {
        // Update modification time if file exists
        entry->modified = time(NULL);
    } else {
        // Create new file
        if (!add_fs_entry(full_path, false)) {
            custom_printf("touch: cannot create file '%s': Filesystem full\n", filename);
        }
    }
}

void cmd_mkdir(const char* dirname) {
    if (!dirname) {
        custom_printf("mkdir: missing operand\n");
        return;
    }

    char full_path[MAX_PATH_SIZE];
    if (dirname[0] == '/') {
        strncpy(full_path, dirname, MAX_PATH_SIZE);
    } else {
        snprintf(full_path, MAX_PATH_SIZE, "%s/%s", current_dir, dirname);
    }

    if (find_fs_entry(full_path)) {
        custom_printf("mkdir: cannot create directory '%s': File exists\n", dirname);
        return;
    }

    if (!add_fs_entry(full_path, true)) {
        custom_printf("mkdir: cannot create directory '%s': Filesystem full\n", dirname);
    }
}

void cmd_rm(const char* path) {
    if (!path) {
        custom_printf("rm: missing operand\n");
        return;
    }

    char full_path[MAX_PATH_SIZE];
    if (path[0] == '/') {
        strncpy(full_path, path, MAX_PATH_SIZE);
    } else {
        snprintf(full_path, MAX_PATH_SIZE, "%s/%s", current_dir, path);
    }

    // Find the entry index
    int entry_index = -1;
    for (int i = 0; i < virtual_fs.count; i++) {
        if (strcmp(virtual_fs.entries[i].name, full_path) == 0) {
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1) {
        custom_printf("rm: cannot remove '%s': No such file or directory\n", path);
        return;
    }

    // Remove entry by shifting remaining entries left
    for (int i = entry_index; i < virtual_fs.count - 1; i++) {
        virtual_fs.entries[i] = virtual_fs.entries[i + 1];
    }
    virtual_fs.count--;
}

void cmd_date() {
    time_t now = time(NULL);
    char* date_str = ctime(&now);
    custom_printf("%s", date_str);
}

void cmd_whoami() {
    custom_printf("guest\n");
}

void cmd_clear() {
    custom_printf("\033[2J\033[H");  // ANSI escape sequence to clear screen
}

void cmd_readme() {
    custom_printf("%s", README_CONTENT);
}

/**
 * Process and execute a command
 */
void process_command(char *input, vect_t *args_vector) {
    if (!input || strlen(input) == 0) {
        return;
    }

    // Initialize virtual filesystem if needed
    init_fs();

    // Tokenize input if args_vector is empty
    bool should_delete_vector = false;
    if (vect_size(args_vector) == 0) {
        args_vector = tokenize_input(input);
        should_delete_vector = true;
    }

    if (vect_size(args_vector) == 0) {
        if (should_delete_vector) {
            vect_delete(args_vector);
        }
        return;
    }

    const char *command = vect_get(args_vector, 0);

    if (strcmp(command, "help") == 0) {
        print_help();
    } else if (strcmp(command, "cd") == 0) {
        if (vect_size(args_vector) > 1) {
            cmd_cd(vect_get(args_vector, 1));
        } else {
            custom_printf("cd: missing argument\n");
        }
    } else if (strcmp(command, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(command, "ls") == 0) {
        cmd_ls(args_vector);
    } else if (strcmp(command, "echo") == 0) {
        cmd_echo(args_vector);
    } else if (strcmp(command, "cat") == 0) {
        if (vect_size(args_vector) > 1) {
            cmd_cat(vect_get(args_vector, 1));
        } else {
            custom_printf("cat: missing file operand\n");
        }
    } else if (strcmp(command, "touch") == 0) {
        if (vect_size(args_vector) > 1) {
            cmd_touch(vect_get(args_vector, 1));
        } else {
            custom_printf("touch: missing file operand\n");
        }
    } else if (strcmp(command, "mkdir") == 0) {
        if (vect_size(args_vector) > 1) {
            cmd_mkdir(vect_get(args_vector, 1));
        } else {
            custom_printf("mkdir: missing operand\n");
        }
    } else if (strcmp(command, "rm") == 0) {
        if (vect_size(args_vector) > 1) {
            cmd_rm(vect_get(args_vector, 1));
        } else {
            custom_printf("rm: missing operand\n");
        }
    } else if (strcmp(command, "date") == 0) {
        cmd_date();
    } else if (strcmp(command, "whoami") == 0) {
        cmd_whoami();
    } else if (strcmp(command, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(command, "readme") == 0) {
        cmd_readme();
    } else if (strcmp(command, "exit") == 0) {
        return;
    } else {
        custom_printf("Unknown command: %s\n", command);
        custom_printf("Type 'help' for a list of commands\n");
    }

    if (should_delete_vector) {
        vect_delete(args_vector);
    }
}
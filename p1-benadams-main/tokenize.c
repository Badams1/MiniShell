#include <stdio.h>

#include "tokenize.h"

/**
 * Is the given character a defined special character?
 */
int is_special_character(char ch) {
  return ch == '(' || ch == ')' || ch == '<' || ch == '>' || ch == ';' || ch == '|'; 
}

/**
 * Read a quoted string handles everything as a single token
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
 * Read a word from the input string
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

int main(int argc, char **argv) {
    char buf[256]; 
    char input[256];
    
    fgets(input, sizeof(input), stdin);

    int i = 0; 
    while (input[i] != '\0') { 
        // Skip spaces between tokens
        if (input[i] == ' ') {
            ++i;
            continue;
        }

        // Handle quoted strings
        if (input[i] == '"') {

            i += read_quoted_string(&input[i], buf); // read everything inside quotes
            printf("\%s\n", buf);
            continue;
        }

        // Handle special characters
        if (is_special_character(input[i])) {
            printf("%c\n", input[i]);
            ++i; 
            continue;
        }

        // Handle regular words (commands or arguments)
        i += read_word(&input[i], buf);
        printf("%s\n", buf);
    }

    return 0;
}

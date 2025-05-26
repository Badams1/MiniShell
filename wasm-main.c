#include "tokenize.h"
#include "vect.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Emscripten-specific defines
#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE __attribute__((used))
#endif

#define MAX_INPUT_SIZE 255
#define MAX_OUTPUT_SIZE 4096

// Global buffer for output
static char g_output_buffer[MAX_OUTPUT_SIZE];
static size_t g_output_pos = 0;

// Custom printf that writes to our buffer
EMSCRIPTEN_KEEPALIVE
int custom_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(g_output_buffer + g_output_pos, 
                          MAX_OUTPUT_SIZE - g_output_pos, 
                          format, 
                          args);
    if (result > 0) {
        g_output_pos += result;
    }
    va_end(args);
    return result;
}

// Forward declaration
void process_command(char* input, vect_t* args_vector);

// Override stdin functions
__attribute__((used))
char* custom_gets(char* str) {
    return NULL;
}

__attribute__((used))
int custom_getchar(void) {
    return EOF;
}

EMSCRIPTEN_KEEPALIVE
const char* process_wasm_command(const char* input) {
    // Clear the output buffer
    g_output_buffer[0] = '\0';
    g_output_pos = 0;
    
    // Process the command
    vect_t* args_vector = vect_new();
    process_command((char*)input, args_vector);
    vect_delete(args_vector);
    
    return g_output_buffer;
}

int main() {
    // WebAssembly initialization
    custom_printf("Welcome! Type 'help' to see available commands.\n");
    return 0;
} 
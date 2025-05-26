#!/bin/bash

# Ensure the output directory exists
mkdir -p wasm-build

# Compile the C code to WebAssembly
emcc shell.c vect.c tokenize.c wasm-main.c \
  -o wasm-build/terminal.js \
  -s WASM=1 \
  -s EXPORTED_FUNCTIONS='["_main", "_process_wasm_command"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
  -s EXIT_RUNTIME=0 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ASSERTIONS=2 \
  -s SAFE_HEAP=1 \
  -s STACK_OVERFLOW_CHECK=2 \
  -g4 \
  -O1

# Copy the output files to the public directory
cp wasm-build/terminal.* ../public/wasm/ 
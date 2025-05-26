CC=gcc
CFLAGS=-g -std=c11
EMCC=emcc
EMFLAGS=-s WASM=1 -s EXPORTED_FUNCTIONS="['_main','_process_wasm_command']" -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" -s EXIT_RUNTIME=0

TOKENIZE_OBJS=$(patsubst %.c,%.o,$(filter-out shell.c wasm-main.c,$(wildcard *.c)))
SHELL_OBJS=$(patsubst %.c,%.o,$(filter-out tokenize.c wasm-main.c,$(wildcard *.c)))
WASM_OBJS=shell.c vect.c tokenize.c wasm-main.c

ifeq ($(shell uname), Darwin)
	LEAKTEST ?= leaks --atExit --
else
	LEAKTEST ?= valgrind --leak-check=full
endif

.PHONY: all valgrind clean test wasm install-wasm

all: shell tokenize

wasm: wasm-build/terminal.js

wasm-build/terminal.js: $(WASM_OBJS)
	mkdir -p wasm-build
	$(EMCC) $(EMFLAGS) -o $@ $^

install-wasm: wasm
	mkdir -p ../public/wasm
	cp wasm-build/terminal.{js,wasm} ../public/wasm/

valgrind: shell tokenize
	$(LEAKTEST) ./tokenize
	$(LEAKTEST) ./shell

tokenize-tests shell-tests : %-tests: %
	env python3 tests/$*_tests.py

test: tokenize-tests shell-tests 

clean: 
	rm -rf *.o
	rm -f shell tokenize
	rm -rf wasm-build
	rm -f ../public/wasm/terminal.{js,wasm}

shell: $(SHELL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

tokenize: $(TOKENIZE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^


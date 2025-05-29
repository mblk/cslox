CC=gcc
LD=gcc
CFLAGS=-c -g -Wall -Wextra #-Werror
LIBS=
LDFLAGS=

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))

all: clean $(OBJ) link

clean:
	mkdir -p obj/
	-rm -f obj/*

obj/%.o: src/%.c
	$(CC) $(CFLAGS) $< -o $@

link:
	$(LD) $(LDFLAGS) $(LIBS) $(OBJ) -o clox

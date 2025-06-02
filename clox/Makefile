
SAN=-fsanitize=address,undefined,bounds,leak,signed-integer-overflow

CC=gcc
LD=gcc

#CC=clang
#LD=clang

CFLAGS=-std=c23 -c -g -Wall -Wextra $(SAN)
LIBS=
LDFLAGS=$(SAN)

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

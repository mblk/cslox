
SAN=-fsanitize=address,undefined,bounds,leak,signed-integer-overflow

CC=gcc
LD=gcc

#CC=clang
#LD=clang

CFLAGS=-c -g -Wall -Wextra $(SAN) #-Werror
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

#
# Makefile for clox
#

# Build mode, use like this:
#   make
#   make BUILD=release
BUILD ?= debug

# Compiler/Linker
CC = gcc
LD = gcc
#CC = clang
#LD = clang

# Flags
#COMMON_CFLAGS = -std=c23 -c -Wall -Wextra -Werror			# too new for githubs ubuntu-latest
COMMON_CFLAGS = -std=c2x -c -Wall -Wextra -Werror
COMMON_LDFLAGS =

ifeq ($(BUILD), release)
	CFLAGS = $(COMMON_CFLAGS) -O3 -DNDEBUG -march=native -flto -fomit-frame-pointer
	LDFLAGS = $(COMMON_LDFLAGS) -flto -Wl,--strip-all
else
	SAN = -fsanitize=address,undefined,bounds,leak,signed-integer-overflow
	CFLAGS = $(COMMON_CFLAGS) -g $(SAN)
	LDFLAGS = $(COMMON_LDFLAGS) $(SAN)
endif

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))

LIBS = -lm
OUT = clox

# Targets
.PHONY: all clean link

all: clean $(OBJ) link

clean:
	mkdir -p obj/
	-rm -f obj/* $(OUT)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) $< -o $@

link:
	$(LD) $(LDFLAGS) $(LIBS) $(OBJ) -o $(OUT)

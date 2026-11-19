
# ==========================================
# Cross-platform raylib Makefile
# macOS / Linux / Windows
# ==========================================

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude

# Detect OS
ifeq ($(OS),Windows_NT)

    PLATFORM = Windows
    EXE = .exe

    RAYLIB_DIR ?= C:/raylib/w64devkit

    RAYLIB_CFLAGS = -I$(RAYLIB_DIR)/include
    RAYLIB_LIBS = -L$(RAYLIB_DIR)/lib -lraylib -lopengl32 -lgdi32 -lwinmm

else

    UNAME_S := $(shell uname -s)

    ifeq ($(UNAME_S),Darwin)

        PLATFORM = macOS
        EXE =

        RAYLIB_DIR ?= $(shell brew --prefix raylib)

        RAYLIB_CFLAGS = -I$(RAYLIB_DIR)/include
        RAYLIB_LIBS = -L$(RAYLIB_DIR)/lib -lraylib \
            -framework OpenGL \
            -framework Cocoa \
            -framework IOKit \
            -framework CoreVideo

    else

        PLATFORM = Linux
        EXE =

        RAYLIB_CFLAGS =
        RAYLIB_LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

    endif
endif

# Source files
LIB_SRC = $(wildcard src/ds_*.c)
LIB_OBJ = $(LIB_SRC:.c=.o)

GAME_SRC = $(filter-out src/ds_%.c, $(wildcard src/*.c))

GAME_BIN = game$(EXE)
TEST_BIN = tests/test_list$(EXE)

# Default target
all: $(GAME_BIN)

# Build game
$(GAME_BIN): $(GAME_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -o $@ $^ $(RAYLIB_LIBS)

# Build and run game
run: $(GAME_BIN)
	./$(GAME_BIN)

# Build and run tests
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/test_list.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile library files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f src/*.o tests/*.exe tests/*.o tests/test_list game game.exe
	rm -rf game.dSYM

# Information
info:
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CC)"
	@echo "Raylib directory: $(RAYLIB_DIR)"
	@echo "Raylib libraries: $(RAYLIB_LIBS)"

.PHONY: all run test clean info

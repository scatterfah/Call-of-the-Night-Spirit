CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -g -Iinclude

LIB_SRC  = $(wildcard src/ds_*.c)
LIB_OBJ  = $(LIB_SRC:.c=.o)
GAME_SRC = $(filter-out src/ds_%.c, $(wildcard src/*.c))

# Detect operating system
ifeq ($(OS),Windows_NT)
    PLATFORM = Windows
else
    PLATFORM = $(shell uname -s)
endif

# Fedora / Linux
ifeq ($(PLATFORM),Linux)
    EXE_EXT = 
    RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib 2>/dev/null)
    RAYLIB_LIBS = $(shell pkg-config --libs raylib 2>/dev/null)
    ifeq ($(strip $(RAYLIB_LIBS)),)
        RAYLIB_LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

# macOS
ifeq ($(PLATFORM),Darwin)
    EXE_EXT =
    RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib 2>/dev/null)
    RAYLIB_LIBS = $(shell pkg-config --libs raylib 2>/dev/null)
    ifeq ($(strip $(RAYLIB_LIBS)),)
        RAYLIB_LIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    endif
endif

# Windows (GCC / MinGW)
ifeq ($(PLATFORM),Windows)
    EXE_EXT = .exe
    RAYLIB_DIR ?= C:/raylib/w64devkit
    RAYLIB_CFLAGS = -I"$(RAYLIB_DIR)/include"
    RAYLIB_LIBS = -L"$(RAYLIB_DIR)/lib" -lraylib -lopengl32 -lgdi32 -lwinmm
endif

GAME_BIN = game$(EXE_EXT)
TEST_BIN = tests/test_ds$(EXE_EXT)

.PHONY: all test game clean

# Default target
all: game-bin

# Build and run data structure tests (no raylib required)
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/test_ds.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run the game
game: game-bin
	./$(GAME_BIN)

# Build the executable using a separate target
game-bin: $(GAME_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -o $(GAME_BIN) $(GAME_SRC) $(LIB_OBJ) $(RAYLIB_LIBS)

# Compile data structure modules
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Remove generated files
clean:
	rm -f src/*.o tests/test_ds tests/test_ds.exe game game.exe

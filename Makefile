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
    RAYLIB_CFLAGS = -I"$(RAYLIB_DIR)/include"
    RAYLIB_LIBS = -L"$(RAYLIB_DIR)/lib" -lraylib -lopengl32 -lgdi32 -lwinmm

else

    UNAME_S := $(shell uname -s)

    ifeq ($(UNAME_S),Darwin)

        PLATFORM = macOS
        EXE =

        RAYLIB_DIR ?= $(shell brew --prefix raylib 2>/dev/null)
        ifneq ($(strip $(RAYLIB_DIR)),)
            RAYLIB_CFLAGS = -I$(RAYLIB_DIR)/include
            RAYLIB_LIBS = -L$(RAYLIB_DIR)/lib -lraylib \
                -framework OpenGL \
                -framework Cocoa \
                -framework IOKit \
                -framework CoreVideo
        else
            RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib 2>/dev/null)
            RAYLIB_LIBS = $(shell pkg-config --libs raylib 2>/dev/null)
            ifeq ($(strip $(RAYLIB_LIBS)),)
                RAYLIB_LIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
            endif
        endif

    else

        PLATFORM = Linux
        EXE =

        RAYLIB_CFLAGS = $(shell pkg-config --cflags raylib 2>/dev/null)
        RAYLIB_LIBS = $(shell pkg-config --libs raylib 2>/dev/null)
        ifeq ($(strip $(RAYLIB_LIBS)),)
            RAYLIB_LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
        else
            RAYLIB_LIBS += -lm
        endif

    endif
endif

# Source files
LIB_SRC = $(wildcard src/ds_*.c)
LIB_OBJ = $(LIB_SRC:.c=.o)
GAME_SRC = $(filter-out src/ds_%.c, $(wildcard src/*.c))

GAME_BIN = game$(EXE)
TEST_SRC = $(firstword $(wildcard tests/test_ds.c tests/test_list.c))
TEST_BIN = $(basename $(TEST_SRC))$(EXE)

.PHONY: all run test clean info

# Default target
all: $(GAME_BIN)

# Build game
$(GAME_BIN): $(GAME_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -o $@ $^ $(RAYLIB_LIBS)

# Build and run game
run: $(GAME_BIN)
	./$(GAME_BIN)

# Build and run data structure tests (no raylib required)
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile data structure modules
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -f src/*.o tests/*.o tests/test_list tests/test_list.exe tests/test_ds tests/test_ds.exe game game.exe
	rm -rf game.dSYM

# Diagnostics
info:
	@echo "Platform:         $(PLATFORM)"
	@echo "Compiler:         $(CC)"
	@echo "Raylib directory: $(RAYLIB_DIR)"
	@echo "Raylib CFLAGS:    $(RAYLIB_CFLAGS)"
	@echo "Raylib libraries: $(RAYLIB_LIBS)"
	@echo "Test source:      $(TEST_SRC)"
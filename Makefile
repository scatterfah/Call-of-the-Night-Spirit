# ==========================================
# Cross-platform raylib Makefile
# macOS / Linux / Windows
#
# make run     build and run the game
# make test    build and run every test in tests/ (no raylib needed)
# make info    show what the Makefile detected
# make clean   delete build files
#
# Different raylib folder?  make run RAYLIB_DIR=D:/path/to/raylib
# ==========================================

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude

# ---------- Detect the operating system ----------
ifeq ($(OS),Windows_NT)

    PLATFORM = Windows
    EXE = .exe

    RAYLIB_DIR ?= C:/raylib/w64devkit
    # -idirafter (not -I): raylib's folder also contains its own copy of the
    # standard headers, which clash with MSYS2's. -idirafter searches it LAST.
    RAYLIB_CFLAGS = -idirafter "$(RAYLIB_DIR)/include"
    RAYLIB_LIBS = -L"$(RAYLIB_DIR)/lib" -lraylib -lopengl32 -lgdi32 -lwinmm

else

    UNAME_S := $(shell uname -s)

    ifeq ($(UNAME_S),Darwin)

        PLATFORM = macOS
        EXE =

        RAYLIB_DIR ?= $(shell brew --prefix raylib 2>/dev/null)
        ifneq ($(strip $(RAYLIB_DIR)),)
            RAYLIB_CFLAGS = -idirafter $(RAYLIB_DIR)/include
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

# ---------- Source files ----------
HEADERS  = $(wildcard include/*.h)
LIB_SRC  = $(wildcard src/ds_*.c)
LIB_OBJ  = $(LIB_SRC:.c=.o)
GAME_SRC = $(filter-out src/ds_%.c, $(wildcard src/*.c))

GAME_BIN = game$(EXE)

# One test per data structure: tests/test_NAME.c is built with src/ds_NAME.c
TEST_SRC  = $(wildcard tests/test_*.c)
TEST_BINS = $(patsubst %.c,%$(EXE),$(TEST_SRC))

# Tests that need another structure's code besides their own.
# Format:  EXTRA_<name> = src/ds_<other>.c
EXTRA_pool = src/ds_stack.c

.PHONY: all run test clean info

# Default target
all: $(GAME_BIN)

# ---------- Game ----------
$(GAME_BIN): $(GAME_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -o $@ $^ $(RAYLIB_LIBS)

run: $(GAME_BIN)
	./$(GAME_BIN)

# ---------- Tests (no raylib required) ----------
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "== $$t"; ./$$t || exit 1; done
	@echo "ALL TESTS PASSED"

tests/test_%$(EXE): tests/test_%.c src/ds_%.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ tests/test_$*.c src/ds_$*.c $(EXTRA_$*)

# ---------- Data structure modules ----------
src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- Clean ----------
clean:
	rm -f src/*.o tests/*.o $(TEST_BINS) game game.exe
	rm -rf game.dSYM

# ---------- Diagnostics ----------
info:
	@echo "Platform:         $(PLATFORM)"
	@echo "Compiler:         $(CC)"
	@echo "Raylib directory: $(RAYLIB_DIR)"
	@echo "Raylib CFLAGS:    $(RAYLIB_CFLAGS)"
	@echo "Raylib libraries: $(RAYLIB_LIBS)"
	@echo "Game sources:     $(GAME_SRC)"
	@echo "Tests:            $(TEST_BINS)"
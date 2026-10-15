CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -Iinclude
LIB_SRC = $(wildcard src/ds_*.c)
LIB_OBJ = $(LIB_SRC:.c=.o)

GAME_SRC = $(filter-out src/ds_%.c, $(wildcard src/*.c))
RAYLIB   = -IC:/raylib/w64devkit/include -LC:/raylib/w64devkit/lib
LIBS     = -lraylib -lopengl32 -lgdi32 -lwinmm

# make test : build and run the data structure tests (no raylib needed)
test: tests/test_ds.exe
	./tests/test_ds.exe

tests/test_ds.exe: tests/test_ds.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# make game : build and run the game (needs raylib)
game: game.exe
	./game.exe

game.exe: $(GAME_SRC) $(LIB_OBJ)
	$(CC) $(CFLAGS) $(RAYLIB) -o $@ $^ $(LIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o tests/*.exe game.exe

.PHONY: test game clean
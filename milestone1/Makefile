PROJDIR = .
SRC_CFILES = $(shell find $(PROJDIR) -type f -name "*.c")
OBJ_CFILES = $(patsubst %.c, %.o, $(SRC_CFILES))
DEFAULT_NVFILE = "heapfile.heap"

DEL = rm -f
CC = gcc
CFLAGS = -std=gnu99 -O0 -Wall -Wextra -ggdb3 
LFLAGS = -lpthread

EXE = crheap_test

.PHONY: all clean run

all: $(EXE)

run: all
	./$(EXE)

commit: clean
	git add *.h *.c Makefile notes.txt
	git commit

push:
	git push origin master

memtest: all
	valgrind --leak-check=full ./$(EXE)

debug: all
	gdb ./$(EXE)

$(EXE): $(OBJ_CFILES)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJ_CFILES) -o $(EXE)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(DEL) $(OBJ_CFILES) $(EXE) $(DEFAULT_NVFILE) vgcore* *.heap

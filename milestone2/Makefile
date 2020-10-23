PROJDIR = .
SRC_CFILES = $(shell find $(PROJDIR) -type f -name "*.c")
OBJ_CFILES = $(patsubst %.c, %.o, $(SRC_CFILES))
GCOV_CFILES = $(patsubst %.c, %.c.gcov, $(SRC_CFILES))
GCDA_CFILES = $(patsubst %.c, %.gcda, $(SRC_CFILES))
GCNO_CFILES = $(patsubst %.c, %.gcno, $(SRC_CFILES))

DEFAULT_NVFILE = "heapfile.heap"

DEL = rm -f
CC = gcc

DEBUG_FLAGS = -fprofile-arcs -ftest-coverage -ggdb3 -O0
WARN_FLAGS = -Wall -Wextra

CFLAGS = -std=gnu99 $(WARN_FLAGS) $(DEBUG_FLAGS)
LFLAGS = -lpthread

EXE = crheap_test

.PHONY: all clean run coverage ctags

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
	@echo "[LINK] $(EXE)"
	@$(CC) $(CFLAGS) $(LFLAGS) $(OBJ_CFILES) -o $(EXE)
	@echo "[BUILD] completed."

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

coverage: run
	@echo "[COV]"
	@gcov $(SRC_CFILES)

ctags:
	ctags -R .

clean:
	@echo "[DEL] OBJ files (*.o)"
	@$(DEL) $(OBJ_CFILES) 
	
	@echo "[DEL] EXE files ($(EXE), *.heap, vgcore*)"
	@$(DEL) $(EXE) $(DEFAULT_NVFILE) vgcore* *.heap 
	
	@echo "[DEL] GCOV files (*.gcov, *.gcda, *.gcno)"
	@$(DEL) $(GCOV_CFILES) $(GCDA_CFILES) $(GCNO_CFILES)
	
	@echo "[CLEAN] completed."

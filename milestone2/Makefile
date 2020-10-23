PROJDIR = .

SRC_CFILES = $(shell find $(PROJDIR) -type f -name "*.c")
SRC_HFILES = $(shell find $(PROJDIR) -type f -name "*.h")
SRC_SUBDIRS = $(shell find $(PROJDIR) -type d)

OBJ_CFILES = $(patsubst %.c, %.o, $(SRC_CFILES))
GCOV_CFILES = $(patsubst %.c, %.c.gcov, $(SRC_CFILES))
GCDA_CFILES = $(patsubst %.c, %.gcda, $(SRC_CFILES))
GCNO_CFILES = $(patsubst %.c, %.gcno, $(SRC_CFILES))

DEFAULT_NVFILE = "heapfile.heap"

DEL = rm -f
CC = gcc

COVERAGE_INFO = coverage.info
COVERAGE_DIR = coverage

DEBUG_FLAGS = -coverage -ggdb3 -O0
WARN_FLAGS = -Wall -Wextra

CFLAGS = -std=gnu99 $(WARN_FLAGS) $(DEBUG_FLAGS) 
LFLAGS = -pthread
IFLAGS = $(addprefix -I, $(SRC_SUBDIRS))

EXE = crheap_test

.PHONY: all clean run coverage ctags

all: $(EXE)

run: all
	./$(EXE)

commit: clean
	git add $(SRC_CFILES) $(SRC_HFILES) Makefile notes.txt
	git commit

push:
	git push origin master

memtest: all
	valgrind --leak-check=full ./$(EXE)

debug: all
	gdb ./$(EXE)

$(EXE): $(OBJ_CFILES)
	@echo "[LINK] $(EXE)"
	@$(CC) $(CFLAGS) $(LFLAGS) $(IFLAGS) $(OBJ_CFILES) -o $(EXE)
	@echo "[BUILD] completed."

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

coverage: run 
	@echo "[GEN_COVERAGE]"
	@lcov --quiet --capture --directory . --output-file $(COVERAGE_INFO)
	@genhtml --quiet $(COVERAGE_INFO) --output-directory $(COVERAGE_DIR)
	@xdg-open $(COVERAGE_DIR)/index.html

ctags:
	ctags -R .

clean:
	@echo "[DEL] OBJ files (*.o)"
	@$(DEL) $(OBJ_CFILES) 
	
	@echo "[DEL] EXE files ($(EXE), *.heap, vgcore*)"
	@$(DEL) $(EXE) $(DEFAULT_NVFILE) vgcore* *.heap 
	
	@echo "[DEL] GCOV files (*.gcov, *.gcda, *.gcno)"
	@$(DEL) $(GCOV_CFILES) $(GCDA_CFILES) $(GCNO_CFILES)

	@# Be careful with this line if you edit it - it uses rm -rf to remove folder
	@echo "[DEL] Coverage info"
	@$(DEL) -r $(COVERAGE_DIR) $(COVERAGE_INFO)
	
	@echo "[CLEAN] completed."

PROJDIR = .

SRC_CFILES = $(shell find $(PROJDIR) -type f -name "*.c")
SRC_ASMFILES = $(shell find $(PROJDIR) -type f -name "*.S")
SRC_HFILES = $(shell find $(PROJDIR) -type f -name "*.h")
SRC_SUBDIRS = $(shell find $(PROJDIR) -type d)

OBJ_CFILES = $(patsubst %.c, %.o, $(SRC_CFILES))
OBJ_ASMFILES = $(patsubst %.S, %.o, $(SRC_ASMFILES))

GCOV_CFILES = $(patsubst %.c, %.c.gcov, $(SRC_CFILES))
GCDA_CFILES = $(patsubst %.c, %.gcda, $(SRC_CFILES))
GCNO_CFILES = $(patsubst %.c, %.gcno, $(SRC_CFILES))

DEFAULT_NVFILE = "heapfile.heap"

DEL = rm -f
CC = gcc
AS = gcc

COVERAGE_INFO = coverage.info
COVERAGE_DIR = coverage

DEBUG_FLAGS = -coverage -ggdb3 -O0 
DISABLED_FLAGS = -fno-optimize-sibling-calls -fno-stack-protector

WARN_FLAGS = -Wall -Wextra

CFLAGS = -std=gnu11 $(WARN_FLAGS) $(DEBUG_FLAGS) $(DISABLED_FLAGS)
LFLAGS = -pthread
IFLAGS = $(addprefix -I, $(SRC_SUBDIRS))

EXE = crheap_test

.PHONY: all clean run coverage ctags

all: $(EXE)

run: all
	./$(EXE)

commit: clean
	git add $(SRC_CFILES) $(SRC_HFILES) $(SRC_ASMFILES) Makefile notes.txt
	git commit

push:
	git push origin master

memtest: all
	valgrind --leak-check=full ./$(EXE)

debug: all
	gdb ./$(EXE)

$(EXE): $(OBJ_CFILES) $(OBJ_ASMFILES)
	@echo "[LINK] $(EXE)"
	@$(CC) $(CFLAGS) $(LFLAGS) $(IFLAGS) $(OBJ_CFILES) $(OBJ_ASMFILES) -o $(EXE)
	@echo "[BUILD] completed."

%.o: %.S
	@echo "[AS] $<"
	@$(AS) $(CFLAGS) $(IFLAGS) -c $< -o $@

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

coverage: run 
	@echo "[GEN_COVERAGE]"
	@lcov --quiet --capture --directory . --output-file $(COVERAGE_INFO)
	@echo "[GEN_REPORT]"
	@genhtml --quiet $(COVERAGE_INFO) --output-directory $(COVERAGE_DIR)
	@echo "[OPEN_REPORT]"
	@xdg-open $(COVERAGE_DIR)/index.html

ctags:
	@echo "GEN"
	ctags -R .

clean:
	@echo "[DEL] OBJ files (*.o)"
	@$(DEL) $(OBJ_CFILES) $(OBJ_ASMFILES)
	
	@echo "[DEL] EXE files ($(EXE), *.heap, vgcore*)"
	@$(DEL) $(EXE) $(DEFAULT_NVFILE) vgcore* *.heap 
	
	@echo "[DEL] GCOV files (*.gcov, *.gcda, *.gcno)"
	@$(DEL) $(GCOV_CFILES) $(GCDA_CFILES) $(GCNO_CFILES)
	
	@# Be careful with this line if you edit it - it uses rm -rf to remove dir
	@echo "[DEL] Coverage info"
	@$(DEL) -r $(COVERAGE_DIR) $(COVERAGE_INFO)
	
	@echo "[CLEAN] completed."

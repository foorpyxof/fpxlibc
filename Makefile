ARGS = -I$(shell pwd)
MAKEFLAGS += --no-print-directory

.PHONY: test debug _test compile compile_dbg setup clean

CC := gcc
CCPLUS := g++
AS := as
LD := ld

test: compile

	@echo "Compiling test programs"
	@find . -type f \( -name "test_*.cpp" \) -exec bash -c 'echo "[CC] {}" && $(CCPLUS) $(ARGS) -std=c++17 -c {} -O3' \;
	@$(MAKE) _test

debug: compile_dbg
	@echo "Compiling test programs"
	@find . -type f \( -name "test_*.cpp" \) -exec bash -c 'echo "[CC] {}" && $(CCPLUS) $(ARGS) -std=c++17 -g -c {} -Og' \;
	@$(MAKE) _test

_test:
	@echo 
	@if ! test -d "./build/testing" ; then \
		mkdir -p ./build/testing; \
	fi

	@mv test_*.o build/unlinked/
	@echo "Linking test programs"
	@cd build/; \
	export CC=$(CC); export CCPLUS=$(CCPLUS); export AS=$(AS); export LD=$(LD); \
	./test_compile.sh
	@echo

compile: CFLAGS := -O2
compile: _compile

compile_dbg: CFLAGS := -g -Og
compile_dbg: ASFLAGS := -g
compile_dbg: _compile

_compile: setup x86_64
	@echo "Compiling source"
	@find . -type f \( -name "*.c" \) -exec bash -c '[ $$(basename {} .c) != test ] && echo "[CC] {}" && $(CC) $(ARGS) $$(if [ -n "$(shell sed -nE 's/asm:(.*)/\1/p' build/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -c {} $(CFLAGS)' \;
	@find . -type f \( -name "*.cpp" -not -name "test_*" \) -exec bash -c '[ $$(basename {} .cpp) != test ] && echo "[CC] {}" && $(CCPLUS) $(ARGS) $$(if [ -n "$(shell sed -nE 's/asm:(.*)/\1/p' build/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -std=c++17 -c {} $(CFLAGS)' \;
	@mv *.o ./build/unlinked/
	@echo

x86_64:
	@if grep -q "asm:x86_64" build/params.fpx; then \
    echo "Assembling source"; \
		find . -type f -name "*.[s|S]" -exec bash -c '[ $$(basename {} | cut -d. -f1) != test ] && echo "[AS] {}" && $(AS) {} $(ASFLAGS) -o build/unlinked/$$(basename {} | cut -d. -f1)-$@.o' \;; \
    echo; \
	fi

setup:
	@if [ $(shell basename $(shell pwd)) != "fpxlibc" ] ; then \
		echo "Must be run from base directory (fpxlibc)"; \
		exit 1; \
	fi

	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi

	@$(MAKE) clean
	@cd build; \
	./assembly.sh

clean:
	@rm build/unlinked/* 2>/dev/null || true
	@rm build/testing/*.out 2>/dev/null || true
	@rm build/*.out 2>/dev/null || true

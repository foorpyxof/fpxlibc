ARGS = -I$(shell pwd)
MAKEFLAGS += --no-print-directory

.PHONY: test debug _test compile compile_dbg setup clean

CC := gcc
CCPLUS := g++
AS := as
LD := ld

test: compile

	@echo "Compiling test programs"
	@find ./testfiles -type f \( -name "*.cpp" \) -exec bash -c 'echo "[CC] {}" && $(CCPLUS) $(ARGS) -c {} -O3' \;
	@$(MAKE) _test

debug: compile_dbg
	@echo "Compiling test programs"
	@find ./testfiles -type f \( -name "*.cpp" \) -exec bash -c 'echo "[CC] {}" && $(CCPLUS) $(ARGS) -g -c {} -Og' \;
	@$(MAKE) _test

_test:
	@echo 
	@if ! test -d "./build/testing" ; then \
		mkdir -p ./build/testing; \
	fi

	@mv *.o build/unlinked/testing/
	@echo "Linking test programs"
	@cd build/; \
	export CC=$(CC); export CCPLUS=$(CCPLUS); export AS=$(AS); export LD=$(LD); \
	./test_compile.sh
	@echo

compile: CFLAGS := -O3
compile: _compile

compile_dbg: CFLAGS := -g -Og
compile_dbg: ASFLAGS := -g
compile_dbg: _compile

_compile: setup x86_64
	@echo "Compiling source"
	@find . -type f \( -name "*.c" \) -exec bash -c 'NAME=$$(basename {} .c); [ $${NAME} != test ] && ([ ! -f build/unlinked/$${NAME}.o ] || [ $$(stat --format=%Y {}) -gt $$(stat --format=%Y build/unlinked/$${NAME}.o) ]) && echo "[CC] {}" && $(CC) $(ARGS) $$(if [ -n "$(shell sed -nE 's/asm:(.*)/\1/p' build/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) --std=c17 -c {} $(CFLAGS)' \;
	@find . -type f \( -name "*.cpp" -not -wholename "*/testfiles/*" \) -exec bash -c 'NAME=$$(basename {} .cpp); [ $${NAME} != test ] && ([ ! -f build/unlinked/$${NAME}.o ] || [ $$(stat --format=%Y {}) -gt $$(stat --format=%Y build/unlinked/$${NAME}.o) ]) && echo "[CC] {}" && $(CCPLUS) $(ARGS) $$(if [ -n "$(shell sed -nE 's/asm:(.*)/\1/p' build/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -std=c++17 -c {} $(CFLAGS)' \;
	@if [ $$(find . -maxdepth 1 -name "*.o" | wc -l) -gt 0 ]; then \
		mv *.o ./build/unlinked/; \
	else \
		echo "No C(++) source to compile or C(++) source not modified!"; \
	fi
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

	@if ! test -d "./build/unlinked/testing" ; then \
		mkdir -p ./build/unlinked/testing; \
	fi

	@cd build; \
	./assembly.sh

#	@$(MAKE) clean

clean:
	@rm build/unlinked/* 2>/dev/null || true
	@rm build/testing/*.out 2>/dev/null || true
	@rm build/*.out 2>/dev/null || true

ARGS = -I$(shell pwd)
MAKEFLAGS += --no-print-directory

.PHONY: test debug _test compile compile_dbg setup clean

CC := gcc
CCPLUS := g++
AS := as
LD := ld

compile: CFLAGS := -O3
compile: FPX_MODE := release
compile: _compile

compile_dbg: CFLAGS := -g -Og
compile_dbg: ASFLAGS := -g
compile_dbg: FPX_MODE := debug
compile_dbg: _compile

test: compile
	@echo "Compiling test programs"
	@find ./testfiles -type f \( -name "*.cpp" \) -exec bash -c 'NAME=$$(basename {} .cpp); echo "[CC] {}" && $(CCPLUS) $(ARGS) -c {} -O3 -o ./build/unlinked/testing/$${NAME}.o' \;
	@$(MAKE) _test

debug: compile_dbg
	@echo "Compiling test programs"
	@find ./testfiles -type f \( -name "*.cpp" \) -exec bash -c 'NAME=$$(basename {} .cpp); echo "[CC] {}" && $(CCPLUS) $(ARGS) -g -c {} -Og -o ./build/unlinked/testing/$${NAME}.o' \;
	@$(MAKE) _test

_test:
	@echo 
	@if ! test -d "./build/testing" ; then \
		mkdir -p ./build/testing; \
	fi

#	@mv *.o build/unlinked/testing/
	@echo "Linking test programs"
	@cd scripts/; \
		export CC="$(CC)"; export CCPLUS="$(CCPLUS)"; export AS="$(AS)"; export LD="$(LD)"; \
	./test_compile.sh
	@echo

_compile: setup x86_64
	@echo "Compiling source"
	@find . -type f \( -name "*.c" \) -exec bash -c ' \
		NAME=$$(basename {} .c); \
		[ $${NAME} != test ] && \
		([ ! -f build/unlinked/$${NAME}.o ] || [ $$(stat --format=%Y {}) -gt $$(stat --format=%Y build/unlinked/$${NAME}.o) ]) && \
		echo "[CC] {}" && \
		$(CC) $(ARGS) $$(if [ "noasm" != "$(shell sed -nE 's/^current_architecture:(.+)$$/\1/p' scripts/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -c {} $(CFLAGS) -o ./build/unlinked/$${NAME}.o \
	' \;; \
	\
	find . -type f \( -name "*.cpp" -not -wholename "*/testfiles/*" \) -exec bash -c ' \
		NAME=$$(basename {} .cpp); \
		[ $${NAME} != test ] && \
		([ ! -f build/unlinked/$${NAME}.o ] || [ $$(stat --format=%Y {}) -gt $$(stat --format=%Y build/unlinked/$${NAME}.o) ]) && \
		echo "[CC] {}" && \
		$(CCPLUS) $(ARGS) $$(if [ "noasm" != "$(shell sed -nE 's/^current_architecture:(.+)$$/\1/p' scripts/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -std=c++17 -c {} $(CFLAGS) -o ./build/unlinked/$${NAME}.o \
	' \;;
#	@if [ ! $$(find . -maxdepth 1 -name "*.o" | wc -l) -gt 0 ]; then \
		echo "No C(++) source to compile or C(++) source not modified!"; \
	fi
	@echo

x86_64:
	@if grep -q "current_architecture:x86_64" scripts/params.fpx; then \
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
	
	@cd scripts/; \
	FPX_MODE="$(FPX_MODE)" ./options.sh

	@if [ "$$(scripts/options.sh CheckClean)" = "clean" ]; then $(MAKE) clean RESET_PARAMS="false"; fi
	@scripts/options.sh SetLast

#	@$(MAKE) clean

clean:
	@if [ -d ./build/ ]; then find ./build/ -type f -exec rm {} +; fi
	@if [ "$(RESET_PARAMS)" != "false" ]; then rm scripts/params.fpx; fi
	@find . -maxdepth 1 -type f -name "*.o" -exec rm {} +

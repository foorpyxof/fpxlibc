ARGS = -I$(shell pwd)
MAKEFLAGS += --no-print-directory

.PHONY: test debug _test compile compile_dbg setup clean _teardown

CC := clang
CCPLUS := clang++
AS := nasm
LD := ld

ASFLAGS := -f elf64

compile: CFLAGS := -O3
compile: FPX_MODE := release
compile: _compile

compile_dbg: CFLAGS := -g -Og -DFPX_DEBUG_ENABLE
compile_dbg: ASFLAGS += -g
compile_dbg: FPX_MODE := debug
compile_dbg: _compile

test: compile
	@echo "Compiling test programs"
	@find ./testfiles -type f \( -name "*.cpp" \) -exec bash -c 'NAME=$$(basename {} .cpp); echo "[CC] {}" && $(CCPLUS) $(ARGS) -c {} $(CFLAGS) -o ./build/unlinked/testing/$${NAME}.o' \;
	@$(MAKE) _test

debug: CFLAGS := -g -Og -DFPX_DEBUG_ENABLE
debug: compile_dbg
	@echo "Compiling test programs"
	@find ./testfiles -type f \( -name "*.cpp" \) -exec bash -c 'NAME=$$(basename {} .cpp); echo "[CC] {}" && $(CCPLUS) $(ARGS) -c {} $(CFLAGS) -o ./build/unlinked/testing/$${NAME}.o' \;
	@$(MAKE) _test

_test:
	@echo
	@if ! test -d "./build/testing" ; then \
		mkdir -p ./build/testing; \
	fi

#	@mv *.o build/unlinked/testing/
	@echo "Linking test programs"
	@cd scripts/; \
		export CC="$(CC)"; export CCPLUS="$(CCPLUS)"; export AS="$(AS)"; export LD="$(LD)"; export CFLAGS="$(CFLAGS)"; export LDFLAGS="$(LDFLAGS)"; \
	./test_compile.sh
	@echo

_compile: setup x86_64
	@echo "Compiling source"
	@find . -type f \( -name "*.c" \) -exec bash -c ' \
		NAME=$$(basename {} .c); \
		[ $${NAME} != test ] && \
		([ ! -f build/unlinked/$${NAME}.o ] || [ $$(stat --format=%Y {}) -gt $$(stat --format=%Y build/unlinked/$${NAME}.o) ]) && \
		echo "[CC] {}" && \
		$(CC) $(ARGS) $$(if [ "noasm" != "$(shell sed -nE 's/^current_architecture:(.+)$$/\1/p' scripts/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -c {} $(CFLAGS) -o ./build/unlinked/tmp/$${NAME}.o && \
		ld -r ./build/unlinked/tmp/$${NAME}.*o -o ./build/unlinked/$${NAME}.o \
	' \;; \
	\
	find . -type f \( -name "*.cpp" -not -wholename "*/testfiles/*" \) -exec bash -c ' \
		NAME=$$(basename {} .cpp); \
		[ $${NAME} != test ] && \
		([ ! -f build/unlinked/$${NAME}.o ] || [ $$(stat --format=%Y {}) -gt $$(stat --format=%Y build/unlinked/$${NAME}.o) ]) && \
		echo "[CC] {}" && \
		$(CCPLUS) $(ARGS) $$(if [ "noasm" != "$(shell sed -nE 's/^current_architecture:(.+)$$/\1/p' scripts/params.fpx)" ]; then echo "-D __FPXLIBC_ASM"; fi) -std=c++17 -c {} $(CFLAGS) -o ./build/unlinked/tmp/$${NAME}.o && \
		ld -r ./build/unlinked/tmp/$${NAME}.*o -o ./build/unlinked/$${NAME}.o \
	' \;;
#	@if [ ! $$(find . -maxdepth 1 -name "*.o" | wc -l) -gt 0 ]; then \
		echo "No C(++) source to compile or C(++) source not modified!"; \
	fi
	@$(MAKE) _teardown
	@echo

_teardown:
	@rm -rf ./build/unlinked/tmp

x86_64:
	@if grep -q "current_architecture:x86_64" scripts/params.fpx; then \
    echo "Assembling source"; \
		find . -type f -name "*.asm" -exec bash -c '[ $$(basename {} | cut -d. -f1) != test ] && echo "[AS] {}" && $(AS) {} $(ASFLAGS) -o build/unlinked/tmp/$$(basename {} | cut -d. -f1).$@.o' \;; \
    echo; \
	fi

setup:
	@echo -e "\033[31;1;4m !\n !\n Did you update assembly code? Be sure to run \`make clean\` first until I fix this!\n !\n !\033[0m";

	@if ! [ -f ./README.md ] && ! [ -f ./LICENSE ] ; then \
		echo "Must be run from base directory"; \
		exit 1; \
	fi

	@if ! test -d "./build/unlinked/testing" ; then \
		mkdir -p ./build/unlinked/testing; \
	fi
	
	@cd scripts/; \
	FPX_MODE="$(FPX_MODE)" ./options.sh

	@if [ "$$(scripts/options.sh CheckClean)" = "clean" ]; then $(MAKE) clean RESET_PARAMS="false"; fi
	@scripts/options.sh SetLast

	@if ! test -d "./build/unlinked/tmp" ; then \
		mkdir -p ./build/unlinked/tmp; \
	fi

#	@$(MAKE) clean

clean:
	@if [ -d ./build/ ]; then find ./build/ -type f -exec rm {} +; fi
	@if [ -d ./build/unlinked/tmp/ ]; then rm -rf ./build/unlinked/tmp/; fi
	@if [ "$(RESET_PARAMS)" != "false" ] && [ -f scripts/params.fpx ]; then rm scripts/params.fpx; fi
	@find . -maxdepth 1 -type f -name "*.o" -exec rm {} +

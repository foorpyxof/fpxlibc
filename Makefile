ARGS = -I$(shell pwd)

.PHONY: test debug _test compile compile_dbg setup clean

test: compile
	find . -type f \( -name "test_*.cpp" \) -exec g++ $(ARGS) -std=c++17 -c {} \;
	@make _test

debug: compile_dbg
	find . -type f \( -name "test_*.cpp" \) -exec g++ $(ARGS) -std=c++17 -g -c {} \;
	@make _test

_test:
	@if ! test -d "./build/testing" ; then \
		mkdir -p ./build/testing; \
	fi

	@mv test_*.o build/unlinked/
	@cd build/; \
	./test_compile.sh

compile: setup
	find . -type f \( -name "*.c" \) -exec gcc $(ARGS) -c {} \;
	find . -type f \( -name "*.cpp" -not -name "test_*" \) -exec g++ $(ARGS) -std=c++17 -c {} \;
	@mv *.o ./build/unlinked/

compile_dbg: setup
	find . -type f \( -name "*.c" \) -exec gcc $(ARGS) -g -c {} \;
	find . -type f \( -name "*.cpp" -not -name "test_*" \) -exec g++ $(ARGS) -std=c++17 -g -c {} \;
	@mv *.o ./build/unlinked/

setup:
	@if [ $(shell basename $(shell pwd)) != "fpxlibc" ] ; then \
		echo "Must be run from base directory (fpxlibc)"; \
		exit 1; \
	fi

	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi

	make clean

clean:
	@(rm build/unlinked/* || true) 2> /dev/null 
	@(rm build/*.out || true) 2> /dev/null 
compile:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

debug:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -g -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out
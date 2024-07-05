compile:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_DEFAULT -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_DEFAULT -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

debug:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_DEFAULT -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_DEFAULT -g -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

tcpserver:
	@if ! test -d "./build/unlinked" ; then \
	mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_TCP_SERVER -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_TCP_SERVER -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

tcpclient:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_TCP_CLIENT -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_TCP_CLIENT -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

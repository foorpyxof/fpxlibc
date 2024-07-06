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
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_TCP_SERVER -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_TCP_SERVER -g -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

tcpclient:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_TCP_CLIENT -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_TCP_CLIENT -g -c {} \;
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out
	@# g++ *.o -o ./build/fpxLIB.out

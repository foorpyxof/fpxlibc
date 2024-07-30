.PHONY: debug tcpserver tcpclient setup breakdown

debug: _setup _debug _breakdown
tcpserver: _setup _tcpserver _breakdown
tcpclient: _setup _tcpclient _breakdown
httpserver: _setup _httpserver _breakdown

_debug:
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_DEFAULT -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_DEFAULT -g -c {} \;

_tcpserver:
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_TCP_SERVER -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_TCP_SERVER -g -c {} \;

_tcpclient:
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_TCP_CLIENT -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_TCP_CLIENT -g -c {} \;

_httpserver:
	find . -type f \( -name "*.c" \) -exec gcc -D __FPX_COMPILE_HTTP_SERVER -g -c {} \;
	find . -type f \( -name "*.cpp" \) -exec g++ -D __FPX_COMPILE_HTTP_SERVER -g -c {} \;

_setup:
	@if ! test -d "./build/unlinked" ; then \
		mkdir -p ./build/unlinked; \
	fi

_breakdown:
	mv *.o ./build/unlinked/
	g++ -I $(shell pwd) ./build/unlinked/*.o -o ./build/fpxLIB.out

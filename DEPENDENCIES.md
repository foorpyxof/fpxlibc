# object files:

tcpclient.o depends on:
- exceptions.o
- string.o

tcpserver.o depends on:
- exceptions.o
- string.o

httpserver.o depends on:
- tcpserver.o
- crypto.o
- endian.o

linkedlist.o depends on:
- exceptions.o

vector.o depends on:
- exceptions.o
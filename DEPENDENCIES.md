# object files:

tcpclient depends on:
- exceptions
- string
- mem

tcpserver depends on:
- exceptions
- string

httpserver depends on:
- tcpserver
- exceptions
- crypto
- endian
- string

linkedlist depends on:
- exceptions

string depends on:
- mem

format depends on:
- math
- endian

crypto depends on:
- endian
- mem

endian depends on:
- mem

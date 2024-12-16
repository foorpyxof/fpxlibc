# Welcome to fpxlib-C :3

just a fun lil' side project i've been working on for a bit.
I'll keep updating it with more stuff as I go along on my journey to learn more C and C++

---

Includes C++ test-programs that you can build using 'make'.

---

## More Makefile instructions:

- ```make debug``` builds the library and all the test programs with debug flags enabled
- ```make compile``` only compile the library
- ```make compile_dbg``` ditto, but with debug flags enabled

**note:** the Makefile will compile and assemble all .c, .cpp, .s and .S files. to prevent test-programs from being compiled, name them test.(extension). The makefile will ignore these files.

## x86_64 assembly

Some parts of the library have been written in x86_64 assembly for optimization purposes.
These files can be assembled by enabling x86_64 assembly when prompted through ```make``` or ```make compile```.

### have fun uwu

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

**note:** the Makefile will compile and assemble all .c, .cpp, .s and .S files. To prevent test-programs from being compiled, name them test.(extension). The makefile will ignore these files.

## x86_64 assembly

Some parts of the library have been written in x86_64 assembly for optimization purposes.
These files can be assembled by enabling x86_64 assembly when prompted through ```make``` or ```make compile```.

### have fun uwu

## Branches

- **main** contains the current, (hopefully) stable build of the library
- **networking** is used for writing any new code for the networking part of
the library (servers, clients, other net-functionality)
- **x86_64** is used for writing x86_64-asm for pre-existing library code, while
x86_64-asm for new code that has yet to be merged into main will be written
in the appropriate branch

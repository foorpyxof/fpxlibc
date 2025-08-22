# How we do it:

- plan N°.1:
Store structs on a separate metadata-heap.

Regions are marked with 'R'
Arena structs are marked with 'A'
```
+------------------+
| RA.............. |
+------------------+
```

The annoying part with this is that we can not (cheaply) keep track of free chunks in this
separate heap when we start freeing arenas or regions.
e.g. the meta-heap will start looking like this
```
+------------------+
| RARRA...RARR..RA |
+------------------+
```

After this happens, it is of course possible to start looking for free spaces, however there is
no cheap route to take here, as far as I am aware, atleast. Especially since you also have to make
sure you do not overwrite other in-use structs on this meta-heap.

- plan N°.2:
Storing arena+region structs right before the data the caller requested.
We do this by mmap'ing 512+16 bytes **more** than the caller actually requested.
In the first 16 bytes we will store the arena struct (which is 16 bytes :O) and all of
the space after that is for storing (512/32 == )16 region structs
```
+-----------------------------------+
| ARRRRR........... | ..userspace.. |
+-----------------------------------+
```

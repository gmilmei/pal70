# Pedagogic Algorithmic Language compiler and runtime

This is essentially a reimplementation of the PAL compiler by R. Mabee
from 12 June 1970 (written in BCPL, see
http://www.softwarepreservation.org/projects/lang/PAL). It generally
follow the original structure. However, the Boehm-Demers-Weiser
garbage collector is used instead of the original one. Many changes
have been made to bring the language more in line with the PAL
reference manual (Arthur Evans, MIT, February 1968), such as built-in
functions and the equivalent use of the (), [], and {} bracket pairs,
while retaining language extensions, such as the `while`, `if`, `test`
constructs, though the `JJ` operation, and thus the `val` or `valof`
constructs, are not yet supported.

## Building and installing

Prerequisites: Boehm GC

Adapt `src/config.h` to set the endianness of the target architecture
and review the compiler flags in `src/Makefile`.

Start build with:

    make

To install:

    make install

The usual install flags `DESTDIR`, `BINDIR` and `MANDIR` are
supported.

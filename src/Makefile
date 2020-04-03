#OPTFLAGS=-O2
# CFLAGS for the Boehm GC
GCCFLAGS=`pkg-config --cflags bdw-gc`
# LDFLAGS for the Boehm GC
GCLDFLAGS=`pkg-config --libs bdw-gc`
CFLAGS=-Wall -std=c99 -D_POSIX_C_SOURCE=200809L -pedantic -g ${GCCFLAGS} ${OPTFLAGS}
LDFLAGS=${GCLDFLAGS} -lm

OBJS=\
	builtins.o \
	code.o \
	disassembler.o \
	error.o \
	interpreter.o \
	list.o \
	parser.o \
	scanner.o \
	stack.o \
	strings.o \
	translator.o \
	tree.o \
        value.o

all: pal70

pal70: pal70.o ${OBJS}

deps:
	gcc -MM *.c *.h > depend

tags:
	etags *.c *.h

clean:
	rm -f pal70 *.o

-include depend

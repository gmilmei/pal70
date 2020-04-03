NAME=pal70-1.0
BINDIR=/usr/bin
MANDIR=/usr/share/man

OPTFLAGS=-O2

all: pal70

pal70:
	${MAKE} -C src pal70 OPTFLAGS="${OPTFLAGS}"

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m0755 src/pal70 $(DESTDIR)$(BINDIR)/pal70
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	install -m0644 man/pal70.1 $(DESTDIR)$(MANDIR)/man1/pal70.1


tar: clean
	mkdir -p ${NAME}
	cp -a AUTHORS emacs examples LICENSE Makefile man README.md src ${NAME}
	tar zcf ${NAME}.tar.gz --owner=0 --group=0 ${NAME}
	rm -rf ${NAME}

clean:
	${MAKE} -C src clean
	${MAKE} -C examples clean

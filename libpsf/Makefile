PREFIX?=/usr/local
CC?=gcc
CFLAGS?=-O3

all: libpsf.so libpsf.a

%.so: libpsf.o
	$(CC) $(CFLAGS) -shared -fPIC $< -o $@

%.a: libpsf.c
	$(CC) $(CFLAGS) -c $< -o libpsf.o
	ar q $@ libpsf.o
	ranlib $@

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

install:
	install -s ./libpsf.so $(PREFIX)/lib
	install -s ./libpsf.a $(PREFIX)/lib
	install ./psf.h $(PREFIX)/include/psf.h

uninstall:
	rm $(PREFIX)/lib/libpsf.so
	rm $(PREFIX)/lib/libpsf.a
	rm $(PREFIX)/include/psf.h

clean:
	rm ./libpsf.so
	rm ./libpsf.a


phony: all install clean uninstall

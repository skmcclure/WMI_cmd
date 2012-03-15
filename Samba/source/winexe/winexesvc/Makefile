CFLAGS=-mno-cygwin -Os
LDFLAGS=-mno-cygwin -s -Os

CC := $(shell uname | grep -qi linux && echo i586-mingw32msvc-gcc || echo gcc)

LD=$(CC)

all: winexesvc_exe.c

winexesvc.exe: winexesvc.o service.o
	$(LD) $(LDFLAGS) -o $@ $^

winexesvc_exe.c: winexesvc.exe bin2c.exe
	./bin2c.exe winexesvc_exe winexesvc.exe > $@

bin2c.exe: bin2c.c
	gcc -s -o $@ $^

clean:
	-@rm *.exe *.o winexesvc_exe.c

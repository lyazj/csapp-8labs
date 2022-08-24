#
# Students' Makefile for the Malloc Lab
#
TEAM = lyazj
VERSION = 0.0.1
HANDINDIR = handin

CC = gcc
CFLAGS = -m32 -Wall -O2 -g -pg

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

mtest: mtest.o mm.o memlib.o
	$(CC) $(CFLAGS) -o mtest mtest.o mm.o memlib.o

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h
mtest.o: mtest.c memlib.h mm.h

handin:
	mkdir -p $(HANDINDIR)
	cp mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

clean:
	rm -f *~ *.o mdriver mtest

.PHONY: handin clean

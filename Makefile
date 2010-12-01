#
# Makefile for the malloc lab driver
#

# set the TEAM name the same as in mm.c
TEAM=xyzzy

# default version number for first handin
VERSION=1

# where mm.c gets copied
HANDINDIR=/afs/umbc.edu/users/c/m/cmsc313/pub/cmsc313_submissions/proj6

# set this to -g for debugging with gdb
DEBUG=

# set this to -pg for using gprof
PROFILE=

# set this to -O2 for optimizing the compiler
OPTIMIZE=

CC = /usr/local/bin/gcc
CFLAGS = -Wall $(DEBUG) $(PROFILE) $(OPTIMIZE)

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

clean:
	rm -f *~ *.o mdriver checkalign

handin:
	cp -v mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

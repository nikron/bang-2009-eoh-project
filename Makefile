####################################################
#  The Makefile, builds the project
####################################################

EXENAME=bang-machine

CC=gcc
COPTS=-Wall -Werror -g -D_REENTRANT -lpthread
GTKOPTS=`pkg-config --cflags --libs gtk+-2.0` -lgthread-2.0

OBJS=bang-com.o bang-net.o bang-signals.o bang-module.o core.o main.o

MAINSRC=src/app/main.c
CORESRC=src/base/core.c
COMSRC=src/base/bang-com.c
NETWORKSRC=src/base/bang-net.c
SIGNALSSRC=src/base/bang-signals.c
MODULESRC=src/base/bang-module.c

.PHONY: doc

$(EXENAME): $(OBJS)
	$(CC) $(COPTS) $(GTKOPTS) $^ -o $(EXENAME)

main.o: $(MAINSRC)
	$(CC) -c $(COPTS) $(GTKOPTS) $^

core.o: $(CORESRC)
	$(CC) -c $(COPTS) $^

bang-com.o: $(COMSRC)
	$(CC) -c $(COPTS) $^

bang-signals.o: $(SIGNALSSRC)
	$(CC) -c $(COPTS) $^

bang-module.o: $(MODULESRC)
	$(CC) -c $(COPTS) $^

bang-net.o: $(NETWORKSRC)
	$(CC) -c $(COPTS) $^

all: $(EXENAME) doc

doc:
	cd doc && doxygen Doxygen

clean:
	rm -f $(EXENAME)
	rm -f *.o
	rm -Rf doc/dox

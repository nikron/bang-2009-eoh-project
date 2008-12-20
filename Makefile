####################################################
#  The Makefile, builds the project
####################################################

EXENAME=bang-machine

CC=gcc
COPTS=-Wall -Werror -g -lpthread

OBJS=bang-net.o bang-signals.o core.o main.o

MAINSRC=src/app/main.c
CORESRC=src/base/core.c
NETWORKSRC=src/base/bang-net.c
SIGNALSSRC=src/base/bang-signals.c

.PHONY: doc

$(EXENAME): $(OBJS)
	$(CC) $(COPTS) $^ -o $(EXENAME)

main.o: $(MAINSRC)
	$(CC) -c $(COPTS) $^

core.o: $(CORESRC)
	$(CC) -c $(COPTS) $^

bang-signals.o: $(SIGNALSSRC)
	$(CC) -c $(COPTS) $^

bang-net.o: $(NETWORKSRC)
	$(CC) -c $(COPTS) $^

all: $(EXENAME) doc

doc:
	cd doc && doxygen Doxygen;

clean:
	rm -f $(EXENAME)
	rm -f *.o
	rm -Rf doc/dox

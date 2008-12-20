EXENAME=bang-machine
CC=gcc
COPTS=-Wall -Werror -g
MAINSRC=src/app/main.c
CORESRC=src/base/core.c src/base/core.h

$(EXENAME): core.o $(MAINSRC)
	$(CC) $(COPTS) $^ -o $(EXENAME)

core.o: $(CORESRC)
	$(CC) -c $(COPTS) $^

clean:
	rm -f $(EXENAME)
	rm -f *.o

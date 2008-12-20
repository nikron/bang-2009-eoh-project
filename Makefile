EXENAME=bang-machine
CC=gcc
COPTS=-Wall -Werror -g
MAINSRC=src/app/main.c
CORESRC=src/base/core.c

$(EXENAME): core.o main.o
	$(CC) $(COPTS) $^ -o $(EXENAME)

main.o: $(MAINSRC)
	$(CC) -c $(COPTS) $^

core.o: $(CORESRC)
	$(CC) -c $(COPTS) $^

clean:
	rm -f $(EXENAME)
	rm -f *.o

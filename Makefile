EXENAME=bang-machine
CC=gcc
COPTS=-Wall -Werror -g -lpthread
MAINSRC=src/app/main.c
CORESRC=src/base/core.c
NETWORKSRC=src/base/bang-net.c

$(EXENAME): bang-net.o core.o main.o
	$(CC) $(COPTS) $^ -o $(EXENAME)

main.o: $(MAINSRC)
	$(CC) -c $(COPTS) $^

core.o: bang-net.o $(CORESRC)
	$(CC) -c $(COPTS) $^

bang-net.o: $(NETWORKSRC)
	$(CC) -c $(COPTS) $^

clean:
	rm -f $(EXENAME)
	rm -f *.o

EXENAME=bang-machine
CC=gcc
COPTS=-Wall -Werror -lmpi -g

$(EXENAME): src/main.c
	$(CC) $(COPTS) $^ -o $(EXENAME)

clean:
	rm -f $(EXENAME)

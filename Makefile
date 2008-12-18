EXENAME=bang-machine
CC=gcc
COPTS=-Wall -Werror -g

$(EXENAME): src/main.c
	$(CC) $(COPTS) $^ -o $(EXENAME)

clean:
	rm -f $(EXENAME)

EXENAME=bang-machine
CC=gcc
COPTS=-Wall -Werror -g

$(EXENAME): src/main.c
	$(CC) $(COPTS) $^

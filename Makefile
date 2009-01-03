####################################################
#  The Makefile, builds the project
####################################################

EXENAME=bang-machine

CC=gcc
COPTS=-Wall -Werror -g -D_REENTRANT -lpthread
GTKOPTS=`pkg-config --cflags --libs gtk+-2.0 --libs openssl ` -lgthread-2.0

OBJS=bang-com.o bang-net.o bang-signals.o bang-module.o bang-module-api.o core.o server-preferences.o main.o
MODULES=test-module.so

MAINSRC=src/app/main.c
SPREFERENCESSRC=src/app/server-preferences.c
CORESRC=src/base/core.c
COMSRC=src/base/bang-com.c
NETWORKSRC=src/base/bang-net.c
SIGNALSSRC=src/base/bang-signals.c
MODULESRC=src/base/bang-module.c
APISRC=src/base/bang-module-api.c

.PHONY: doc modules

all: $(EXENAME) modules

$(EXENAME): $(OBJS)
	$(CC) $(COPTS) $(GTKOPTS) $^ -o $(EXENAME)

main.o: $(MAINSRC)
	$(CC) -c $(COPTS) $(GTKOPTS) $^

server-preferences.o: $(SPREFERENCESSRC)
	$(CC) -c $(COPTS) $(GTKOPTS) $^

core.o: $(CORESRC)
	$(CC) -c $(COPTS) $^

bang-com.o: $(COMSRC)
	$(CC) -c $(COPTS) $^

bang-signals.o: $(SIGNALSSRC)
	$(CC) -c $(COPTS) $^

bang-module.o: $(MODULESRC)
	$(CC) -c $(COPTS) $^

bang-module-api.o: $(APISRC)
	$(CC) -c $(COPTS) $^

bang-net.o: $(NETWORKSRC)
	$(CC) -c $(COPTS) $^

modules: $(MODULES)

test-module.so: src/modules/test-module.c
	$(CC) --shared -Wl,-soname,testmodule.so  $(COPTS) $^ -o test-module.so


doc:
	cd doc && doxygen Doxygen

clean:
	rm -f $(EXENAME)
	rm -f *.o
	rm -f *.so
	rm -Rf doc/dox

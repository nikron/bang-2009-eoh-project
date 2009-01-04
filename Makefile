####################################################
#  The Makefile, builds the project
####################################################

EXENAME=bang-machine

CC=gcc
COPTS=-Wall -Werror -g -D_REENTRANT -lpthread
GTKOPTS=`pkg-config --cflags --libs gtk+-2.0 --libs openssl ` -lgthread-2.0
OBJEXT=.o

LOBJS=bang-com$(OBJEXT) bang-net$(OBJEXT) bang-signals$(OBJEXT) bang-module$(OBJEXT) bang-module-api$(OBJEXT) bang-core$(OBJEXT)
AOBJS=server-preferences.o main.o

LIBRARIES=libbang.so $(MODULES)
MODULES=test-module.so libbang.so

MAINSRC=src/app/main.c
SPREFERENCESSRC=src/app/server-preferences.c
CORESRC=src/base/bang-core.c
COMSRC=src/base/bang-com.c
NETWORKSRC=src/base/bang-net.c
SIGNALSSRC=src/base/bang-signals.c
MODULESRC=src/base/bang-module.c
APISRC=src/base/bang-module-api.c

.PHONY: doc modules

all: $(EXENAME) $(LIBRARIES)

$(EXENAME): $(LOBJS) $(AOBJS)
	$(CC) $(COPTS) $(GTKOPTS) $^ -o $(EXENAME)

main.o: $(MAINSRC)
	$(CC) -c $(COPTS) $(GTKOPTS) $^

server-preferences.o: $(SPREFERENCESSRC)
	$(CC) -c $(COPTS) $(GTKOPTS) $^

libbang.so: $(LOBJS)
	$(CC) -shared -Wl,-soname,$@ $(COPTS) $^ -o $@

bang-core.o: $(CORESRC)
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
	$(CC) -shared -Wl,-soname,$@  $(COPTS) $^ -o $@

doc:
	cd doc && doxygen Doxygen

clean:
	rm -f $(EXENAME)
	rm -f *.o
	rm -f *.so
	rm -Rf doc/dox

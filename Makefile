####################################################
#  The Makefile, builds the project
####################################################

EXENAME=bang-machine

CC=gcc

OBJEXT=.o
SRCEXT=.c

COPTS=-Wall -Wextra -Werror -g -D_REENTRANT -lpthread

GTKOPTS=`pkg-config --cflags --libs gtk+-2.0 --libs openssl --libs uuid --libs sqlite3 --libs gthread-2.0`
LOBJS=bang-com$(OBJEXT) \
      bang-net$(OBJEXT) \
      bang-signals$(OBJEXT) \
      bang-module$(OBJEXT) \
      bang-module-api$(OBJEXT) \
      bang-core$(OBJEXT) \
      bang-utils$(OBJEXT) \
      bang-routing$(OBJEXT)

LSRC=src/base/bang-com$(SRCEXT) \
     src/base/bang-net$(SRCEXT) \
     src/base/bang-signals$(SRCEXT) \
     src/base/bang-module$(SRCEXT) \
     src/base/bang-module-api$(SRCEXT) \
     src/base/bang-core$(SRCEXT) \
     src/base/bang-utils$(SRCEXT) \
     src/base/bang-routing$(SRCEXT)

AOBJS=server-preferences$(OBJEXT) \
      main$(OBJEXT)

ASRC=src/app/server-preferences$(SRCEXT) \
      src/app/main$(SRCEXT)

LIBRARIES=libbang.so $(MODULES)
MODULES=test-module.so matrix-mult-module.so

.PHONY: doc modules commit

all: $(EXENAME) $(LIBRARIES)

$(EXENAME): $(LOBJS) $(AOBJS)
	$(CC) $(COPTS) $(GTKOPTS) $^ -o $(EXENAME)

$(AOBJS): $(ASRC)
	$(CC) $(COPTS) $(GTKOPTS) -c $^

libbang.so: $(LOBJS)
	$(CC) -shared -Wl,-soname,$@ $(COPTS) $^ -o $@

$(LOBJS): $(LSRC)
	$(CC) $(COPTS) -c $^

modules: $(MODULES)

test-module.so: src/modules/test-module.c
	$(CC) -shared -Wl,-soname,$@ $(COPTS) $^ -o $@

matrix-mult-module.so: src/modules/matrix-mult-module.c
	$(CC) -shared -Wl,-soname,$@ $(COPTS) $(GTKOPTS) $^ -o $@

doc:
	cd doc && doxygen Doxygen

clean:
	rm -f $(EXENAME) $(LOBJS) $(AOBJS) $(LIBRARIES)
	rm -Rf doc/dox

commit:
	git commit -a
	git svn dcommit
	git push

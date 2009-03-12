####################################################
#  The Makefile, builds the project
####################################################

EXENAME=bang-machine

CC=gcc

OBJEXT=.o
SRCEXT=.c

COPTS=-Wall -Wextra -Werror -fPIC -g -D_REENTRANT -lpthread

GTKOPTS=`pkg-config --cflags --libs gtk+-2.0 --libs openssl --libs uuid --libs gthread-2.0`
LOBJS=bang-com$(OBJEXT) \
      bang-net$(OBJEXT) \
      bang-signals$(OBJEXT) \
      bang-module$(OBJEXT) \
      bang-module-api$(OBJEXT) \
      bang-core$(OBJEXT) \
      bang-utils$(OBJEXT) \
      bang-routing$(OBJEXT) \
      bang-write-thread$(OBJEXT) \
      bang-read-thread$(OBJEXT) \
      bang-peer$(OBJEXT) \
      bang-scan$(OBJEXT) \
      bang-module-registry$(OBJEXT)

LSRC=src/base/bang-com$(SRCEXT) \
     src/base/bang-net$(SRCEXT) \
     src/base/bang-signals$(SRCEXT) \
     src/base/bang-module$(SRCEXT) \
     src/base/bang-module-api$(SRCEXT) \
     src/base/bang-core$(SRCEXT) \
     src/base/bang-utils$(SRCEXT) \
     src/base/bang-routing$(SRCEXT) \
     src/base/bang-read-thread$(SRCEXT) \
     src/base/bang-write-thread$(SRCEXT) \
     src/base/bang-peer$(SRCEXT) \
     src/base/bang-scan$(SRCEXT) \
     src/base/bang-module-registry$(SRCEXT)

AOBJS=preferences$(OBJEXT) \
      main$(OBJEXT) \
      statusbar$(OBJEXT)\
      server-menu$(OBJEXT) \
      bang-machine-utils$(OBJEXT) \
      menus$(OBJEXT)\
      file-menu$(OBJEXT)

ASRC=src/app/preferences$(SRCEXT) \
      src/app/main$(SRCEXT) \
      src/app/statusbar$(SRCEXT) \
      src/app/server-menu$(SRCEXT) \
      src/app/bang-machine-utils$(SRCEXT)\
      src/app/menus$(SRCEXT)\
      src/app/file-menu$(SRCEXT)

LIBRARIES=libbang.so $(MODULES)
MODULES=test-module.so matrix-mult-module.so

.PHONY: doc modules commit

all: $(EXENAME) $(LIBRARIES) menus.xml

$(EXENAME): $(LOBJS) $(AOBJS)
	$(CC) $(COPTS) $(GTKOPTS) $^ -o $(EXENAME)

menus.xml:
	cp src/app/menus.xml .

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
	rm -f menus.xml

commit:
	git commit -a
	git svn dcommit
	git push

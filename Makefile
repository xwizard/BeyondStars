PREFIX=/usr/local
DATADIR=$(PREFIX)/share/starvoyager
DOCDIR=$(PREFIX)/share/doc/starvoyager
BINDIR=$(PREFIX)/bin
VERSION=0.4.4

CPPC=c++
CC=cc
LIBS:=`sdl-config --libs` -lSDL_net -lstdc++
CFLAGS:=`sdl-config --cflags` -Wall -ggdb3
#CFLAGS:=`sdl-config --cflags` -ggdb3 -Wall -Werror -ansi -pedantic
PACKAGENAME=starvoyager-$(VERSION)-`uname -m`-`uname|tr [A-Z] [a-z]`.bin
.SUFFIXES: .c .cc

all: starvoyager

#Linking
starvoyager: alliance.o camera.o database.o error.o game.o interface.o presence.o ship.o sound.o ticker.o calc.o client.o equip.o frag.o graphic.o planet.o server.o sockhelper.o sv.o player.o os.o SDL_rotozoom.o SDL_gfxPrimitives.o
	$(CC) -o starvoyager *.o $(LIBS)

#Include dependencies
*.o: *.h

#Compiling
SDL_rotozoom.o: SDL_rotozoom.c
	$(CC) $(CFLAGS) -c -o SDL_rotozoom.o SDL_rotozoom.c

SDL_gfxPrimitives.o: SDL_gfxPrimitives.c
	$(CC) $(CFLAGS) -c -o SDL_gfxPrimitives.o SDL_gfxPrimitives.c

.cc.o:
	$(CPPC) $(CFLAGS) -DPOSIX -DVERSION=\"${VERSION}\" -DDATADIR=\"${DATADIR}\" -c -o $@ $<

	
#Installing
install: all
	rm $(DATADIR) -rf
	rm $(DOCDIR) -rf
	mkdir -p $(BINDIR) $(DOCDIR) $(DATADIR)/gfx $(DATADIR)/snd 
	cp data/gfx/* $(DATADIR)/gfx/
	cp data/snd/* $(DATADIR)/snd/
	cp data/*.svd $(DATADIR)/
	cp README FAQ LGPL manual.html LICENCE $(DOCDIR)/
	cp starvoyager $(BINDIR)/
	chmod 755 $(BINDIR)/starvoyager

#Uninstalling
uninstall:
	rm -r $(DATADIR)
	rm -r $(DOCDIR)
	rm $(BINDIR)/starvoyager

#Clean
clean:
	rm -f *.o
	rm -f starvoyager
	rm -f starvoyager-*

#Making a binary package
binary:
	cp binpackage.sh $(PACKAGENAME)
	tar czf - $(BINDIR)/starvoyager $(DATADIR) $(DOCDIR) >>$(PACKAGENAME)

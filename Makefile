## file Makefile in basixmo
## by Basile Starynkevitch <basile@starynkevitch.net>
CXX=g++
GCC=gcc
ASTYLE= astyle
MD5SUM= md5sum
SQLITE= sqlite3
PACKAGES= sqlite3 jsoncpp Qt5Core Qt5Widgets Qt5Gui Qt5Sql
PKGCONFIG= pkg-config
CXXOPTIMFLAGS= -g2 -O1
CXXWARNFLAGS= -Wall -Wextra
CXXPREPROFLAGS= -I /usr/local/include  $(shell $(PKGCONFIG) --cflags $(PACKAGES))  -I $(shell $(GCC) -print-file-name=include/)
CXXFLAGS= -std=gnu++14 $(CXXOPTIMFLAGS) $(CXXWARNFLAGS) $(CXXPREPROFLAGS)

ASTYLEFLAGS= --style=gnu -s2  --convert-tabs
CXXSOURCES= $(wildcard *.cc)
## this xmastate basename is "sacred", don't change it
BASIXMO_STATE=basixmo_state
SHELLSOURCES= $(sort $(wildcard [a-z]*.sh))
OBJECTS= $(patsubst %.cc,%.o,$(CXXSOURCES))
GENERATED_HEADERS= $(wildcard _*.h)
LIBES= -L/usr/local/lib  $(shell $(PKGCONFIG) --libs $(PACKAGES)) -pthread  $(shell $(GCC) -print-file-name=libbacktrace.a) 
.PHONY: all checkgithooks installgithooks clean dumpstate restorestate indent
all: checkgithooks bxmo

_timestamp.c: Makefile | $(OBJECTS)
	@echo "/* generated file _timestamp.c - DONT EDIT */" > _timestamp.tmp
	@date +'const char basixmo_timestamp[]="%c";' >> _timestamp.tmp
	@(echo -n 'const char basixmo_lastgitcommit[]="' ; \
	   git log --format=oneline --abbrev=12 --abbrev-commit -q  \
	     | head -1 | tr -d '\n\r\f\"' ; \
	   echo '";') >> _timestamp.tmp
	@(echo -n 'const char basixmo_lastgittag[]="'; (git describe --abbrev=0 --all || echo '*notag*') | tr -d '\n\r\f\"'; echo '";') >> _timestamp.tmp
	(echo -n 'const char*const basixmo_cxxsources[]={'; for sf in $(CXXSOURCES) ; do \
	  echo -n "\"$$sf\", " ; done ; \
	echo '(const char*)0};') >> _timestamp.tmp
	@(echo -n 'const char*const basixmo_shellsources[]={'; for sf in $(SHELLSOURCES) ; do \
	  echo -n "\"$$sf\", " ; done ; \
	echo '(const char*)0};') >> _timestamp.tmp
	@(echo -n 'const char basixmo_statebase[]="'; echo -n $(BASIXMO_STATE); echo -n '";') >> _timestamp.tmp
	@echo >> _timestamp.tmp
	@echo >> _timestamp.tmp
	mv _timestamp.tmp _timestamp.c

$(OBJECTS): basixmo.h $(GENERATED_HEADERS)
bxmo: $(OBJECTS) _timestamp.o
	@if [ -f $@ ]; then echo -n makebackup old executable: ' ' ; mv -v $@ $@~ ; fi
	$(LINK.cc)  $(LINKFLAGS) $(OPTIMFLAGS) -rdynamic $(OBJECTS)  _timestamp.o $(LIBES) -o $@ 

#%.o: %.cc basixmo.h $(GENERATED_HEADERS) | $(shell grep -q $(basename $<)-moc.h $< && echo $(basename $<)-moc.h)

clean:
	$(RM) *~ *% *.o *.so */*.so *.log */*~ */*.orig *.i *.ii *.orig README.html *#
	$(RM) core*
	$(RM) _timestamp.* bxmo

checkgithooks:
	@for hf in *-githook.sh ; do \
	  [ ! -d .git -o -L .git/hooks/$$(basename $$hf "-githook.sh") ] \
	    || (echo uninstalled git hook $$hf "(run: make installgithooks)" >&2 ; exit 1) ; \
	done
installgithooks:
	for hf in *-githook.sh ; do \
	  ln -sv  "../../$$hf" .git/hooks/$$(basename $$hf "-githook.sh") ; \
	done

dumpstate: $(BASIXMO_STATE).sqlite | basixmo-dump-state.sh
	./basixmo-dump-state.sh $(BASIXMO_STATE).sqlite $(BASIXMO_STATE).sql

restorestate: | $(BASIXMO_STATE).sqlite
	@if [ -f $(BASIXMO_STATE).sqlite ]; then \
	  echo makebackup old: ' ' ; mv -b -v  $(BASIXMO_STATE).sqlite  $(BASIXMO_STATE).sqlite~ ; fi
	$(SQLITE) $(BASIXMO_STATE).sqlite < $(BASIXMO_STATE).sql
	touch -r $(BASIXMO_STATE).sql -c $(BASIXMO_STATE).sqlite

indent:
	$(ASTYLE) $(ASTYLEFLAGS) basixmo.h
	for g in $(wildcard [a-z]*.cc) ; do \
	  $(ASTYLE) $(ASTYLEFLAGS) $$g ; \
	done



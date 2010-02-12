# XDD Makefile
SHELL 	=	/bin/sh
OS 		= 	$(shell uname)
DATESTAMP =	$(shell date +%m%d%y )
BUILD 	=	$(shell date +%H%M )
PROJECT =	xdd
VERSION =	7.0.0.rc11
XDD_VERSION = $(OS).$(VERSION).$(DATESTAMP).Build.$(BUILD)
XDDVERSION = \"$(XDD_VERSION)\"
OBJECTS =	access_pattern.o \
		barrier.o \
		end_to_end.o \
		global_time.o \
		heartbeat.o \
		initialization.o \
		io_loop.o \
		io_loop_after_io_operation.o \
		io_loop_after_loop.o  \
		io_loop_before_io_operation.o \
		io_loop_before_loop.o \
		io_loop_perform_io_operation.o \
		io_thread.o \
		io_thread_cleanup.o \
		io_thread_init.o \
		parse.o \
		parse_func.o \
		parse_table.o \
		pclk.o \
		read_after_write.o \
		restart.o \
		results_display.o \
		results_manager.o \
		sg.o \
		ticker.o \
		timestamp.o \
		verify.o \
		xdd.o 

HEADERS = 	access_pattern.h \
		barrier.h \
		end_to_end.h \
		datapatterns.h \
		misc.h \
		parse.h \
		pclk.h \
		ptds.h \
		read_after_write.h \
		restart.h \
		results.h \
		sg.h \
		sg_err.h \
		sg_include.h \
		ticker.h \
		timestamp.h \
		xdd.h \
		xdd_common.h \
		xdd_version.h

TSOBJECTS =	timeserver.o pclk.o ticker.o
GTOBJECTS =     gettime.o global_time.o pclk.o ticker.o

CFLAGS =	-DXDD_VERSION=$(XDDVERSION) -DLINUX -O2 -g
CC = 		gcc
LIBRARIES =	-lpthread

$(info Making xdd for $(OS))
ifeq '$(OS)' 'Linux'
CFLAGS =	-DXDD_VERSION=$(XDDVERSION) -DLINUX -O2 -DSG_IO -D__INTEL__ -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g -fno-strict-aliasing -Wall
endif
ifeq '$(OS)' 'Darwin' 
CFLAGS =	-DXDD_VERSION=$(XDDVERSION) -DOSX -O2 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g
OS = 		OSX
endif
ifeq '$(OS)' 'FreeBSD' 
CFLAGS =	-DXDD_VERSION=$(XDDVERSION) -DFREEBSD -O2 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g
endif
ifeq '$(OS)' 'Solaris' 
CC = 		cc
CFLAGS =	-DXDD_VERSION=$(XDDVERSION) -DSOLARIS -g
LIBRARIES =	-lsocket -lnsl -lpthread  -lxnet -lposix4 -v 
endif
ifeq '$(OS)' 'AIX'
CC =            gxlc
CFLAGS =        -DXDD_VERSION=$(XDDVERSION) -DAIX -g
LIBRARIES =     -lnsl -lpthread  -lxnet -v
endif

all:	xdd timeserver gettime

xdd: 		$(OBJECTS)
	$(CC)  -o xdd $(CFLAGS) $(OBJECTS) $(LIBRARIES)
	mv -f xdd bin/xdd.$(OS)
	rm -f bin/xdd
	ln bin/xdd.$(OS) bin/xdd

timeserver:	$(TSOBJECTS) 
	$(CC)  -o timeserver $(CFLAGS) $(TSOBJECTS) $(LIBRARIES)
	mv -f timeserver bin/timeserver.$(OS)
	rm -f bin/timeserver
	ln bin/timeserver.$(OS) bin/timeserver

gettime:	$(GTOBJECTS) 
	$(CC)  -o gettime $(CFLAGS) $(GTOBJECTS) $(LIBRARIES)
	mv -f gettime bin/gettime.$(OS)
	rm -f bin/gettime
	ln bin/gettime.$(OS) bin/gettime

baseversion:
	echo "#define XDD_BASE_VERSION $(XDDVERSION)" > xdd_base_version.h

dist:	clean baseversion tarball
		echo "Base Version $(XDDVERSION) Source Only"

fulldist:	clean baseversion all tarball
		echo "Base Version $(XDDVERSION) built"

tarball:
	tar cfz ../$(PROJECT).$(XDD_VERSION).tgz .

oclean:
	$(info Cleaning the $(OS) OBJECT files )
	rm -f $(OBJECTS) $(TSOBJECTS) $(GTOBJECTS)

clean: oclean
	$(info Cleaning the $(OS) executable files )
	rm -f bin/xdd.$(OS) \
		bin/xdd \
		bin/timeserver.$(OS) \
		bin/timeserver \
		bin/gettime.$(OS) \
		bin/gettime \
		a.out 
	rm -rf doc/doxygen

install: clean all fastinstall

fastinstall: all
	rm -f /sbin/xdd.$(OS) /sbin/xdd
	cp bin/xdd.$(OS) /sbin
	ln /sbin/xdd.$(OS) /sbin/xdd
	rm -f /sbin/timeserver.$(OS) /sbin/timeserver
	cp bin/timeserver.$(OS) /sbin
	ln /sbin/timeserver.$(OS) /sbin/timeserver
	rm -f /sbin/gettime.$(OS) /sbin/gettime
	cp bin/gettime.$(OS) /sbin
	ln /sbin/gettime.$(OS) /sbin/gettime

doc:
	doxygen doc/Doxyfile
	cd doc/doxygen/latex && make pdf

.PHONY: doc

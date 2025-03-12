# defines SWATDBDIR
include make.conf

# path to SwatDB library files
LIBDIR = $(SWATDBDIR)lib/

# paths to include directories
INCLUDES = -I. -I$(SWATDBDIR)include/

# compiler
CC = g++

# compiler flags for test code build
CFLAGS =  -g -Wall #-pthread

# lflags for linking
LFLAGS = -L$(LIBDIR)


# SwatDB libraries needed to link in to buf manager test
#LIBS = $(LFLAGS) diskmgr.o page.o exceptions.o file.o filemgr.o catalog.o swatdb.o -lm
#LIBS = $(LFLAGS) $(LIBDIR)diskmgr.o $(LIBDIR)page.o $(LIBDIR)exceptions.o $(LIBDIR)file.o $(LIBDIR)filemgr.o $(LIBDIR)catalog.o $(LIBDIR)swatdb.o $(LIBDIR)heapfile.o $(LIBDIR)bufmgr.o $(LIBDIR)data.o $(LIBDIR)record.o  -lm

LIBS = $(LFLAGS) -l swatdb


SRCS = heappage.cpp heappagescanner.cpp

# suffix replacement rule
OBJS = $(SRCS:.cpp=.o)

# be very careful to not add any spaces to ends of these
TARGET = sandbox
UNITTESTS = unittests
CHKPT = chkpt

# gcov unittest version
GCOVUNIT = gcovunit

# generic makefile
.PHONY: clean gcov

all: $(TARGET) $(UNITTESTS) $(STARTUNIT) $(CHKPT)

$(TARGET): $(OBJS) $(TARGET).cpp heappage.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(TARGET).cpp $(OBJS) $(LIBS)

$(UNITTESTS): $(OBJS) $(UNITTESTS).cpp heappage.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $(UNITTESTS) $(UNITTESTS).cpp  -lUnitTest++ $(OBJS) $(LIBS)

$(CHKPT): $(OBJS) $(CHKPT).cpp heappage.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $(CHKPT) $(CHKPT).cpp  -lUnitTest++ $(OBJS) $(LIBS)

gcov:  
	$(CC) -fprofile-arcs -ftest-coverage $(CFLAGS) $(INCLUDES) -o $(GCOVUNIT) $(UNITTESTS).cpp  heappage.cpp heappagescanner.cpp -lUnitTest++  $(LIBS)


# suffix replacement rule using autmatic variables:
# automatic variables: $< is the name of the prerequiste of the rule
# (.cpp file),  and $@ is name of target of the rule (.o file)
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

runtests: $(UNITTESTS)
	./$(UNITTESTS)

clean:
	$(RM) *.o $(TARGET) $(UNITTESTS) $(CHKPT) $(GCOVUNIT) *.gcov *.gcna *.gcno *.gcda *.gcdo

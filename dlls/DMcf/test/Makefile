#updated by kobe, 2008,1,23
#--------------------------------------------------------------------------------------------------
CC = gcc -Wall -g -O3
CXX= g++ -Wall -g -O3
CXXFLAGS=-I/usr/home/conglei/DCF/include -I. -I../ -L. -L/usr/local/lib -L/usr/lib -L/usr/home/conglei/DCF/lib
LIBS=-lpthread -ldl -lboost_serialization
#--------------------------------------------------------------------------------------------------
OUTPUTFILES=test
all: $(OUTPUTFILES)
.SUFFIXES: .o .cpp .hpp
#--------------------------------------------------------------------------------------------------

test: main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CXXFLAGS) -c $<
#--------------------------------------------------------------------------------------------------

clean:
	rm -rf $(OUTPUTFILES) *.o


#updated by kobe, 2008,1,23
#--------------------------------------------------------------------------------------------------
CC = gcc -Wall -g -O3
CXX= g++ -Wall -g -O3
CXXFLAGS=-I../include -I. -L/usr/local/lib -L/usr/lib -L../lib
LIBS=-lpthread -ltransdoc -lmonitor -ldispatch -lcommon -ldl -levent
COMMON_PATN=../master/
#--------------------------------------------------------------------------------------------------
OUTPUTFILES=slave
all: $(OUTPUTFILES)
.SUFFIXES: .o .cpp .hpp
#--------------------------------------------------------------------------------------------------

slave: main.o SlaveConfigInfor.o $(COMMON_PATN)Common.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
../master/Common.o:
	$(CXX) $(CXXFLAGS) -c $(COMMON_PATN)Common.cpp  -o $(COMMON_PATN)Common.o
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CXXFLAGS) -c $<
#--------------------------------------------------------------------------------------------------

clean:
	rm -rf $(OUTPUTFILES) *.o $(COMMON_PATN)Common.o *.so *.a

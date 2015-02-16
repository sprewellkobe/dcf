#updated by kobe, 2008,1,23
#--------------------------------------------------------------------------------------------------
CC = gcc -Wall -g -O3
CXX= g++ -Wall -g -O3
CXXFLAGS=-I../include -I. -L/usr/local/lib -L/usr/lib -L../lib
LIBS=-lpthread -ltransdoc -lmonitor -ldispatch -lcommon -ldl -levent
COMMON_PATH=../master/
#--------------------------------------------------------------------------------------------------
OUTPUTFILES=reducer
all: $(OUTPUTFILES)
.SUFFIXES: .o .cpp .hpp
#--------------------------------------------------------------------------------------------------

reducer: main.o $(COMMON_PATH)Common.o ReducerConfigInfor.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
$(COMMON_PATH)Common.o:
	$(CXX) $(CXXFLAGS) -c $(COMMON_PATH)Common.cpp -o $(COMMON_PATH)Common.o
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CXXFLAGS) -c $<
#--------------------------------------------------------------------------------------------------

clean:
	rm -rf $(OUTPUTFILES) *.o $(COMMON_PATH)Common.o *.so *.a

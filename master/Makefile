#updated by kobe, 2008,1,23
#--------------------------------------------------------------------------------------------------
CC = gcc -Wall -g -O3
CXX= g++ -Wall -g -O3
CXXFLAGS=-I../include -I. -L/usr/local/lib -L/usr/lib -L../lib
LIBS=-lpthread -ltransdoc -lmonitor -ldispatch -lcommon -ldl -levent
#--------------------------------------------------------------------------------------------------
OUTPUTFILES=master console #map.so task.so reduce.so
all: $(OUTPUTFILES)
.SUFFIXES: .o .cpp .hpp
#--------------------------------------------------------------------------------------------------

master: main.o Common.o ConfigInfor.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
console: console.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
map.so: MapDLL.o
	$(CXX) -shared -Wl,-soname,$@ -o $@ $^
task.so: TaskDLL.o Common.o
	$(CXX) -shared -Wl,-soname,$@ -o $@ $^
reduce.so: ReduceDLL.o
	$(CXX) -shared -Wl,-soname,$@ -o $@ $^ 
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CXXFLAGS) -c $<
#--------------------------------------------------------------------------------------------------

clean:
	rm -rf $(OUTPUTFILES) *.o *.so *.a

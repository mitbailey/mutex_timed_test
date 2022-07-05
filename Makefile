CXX = g++
CC = gcc
CPPOBJS = src/cppmain.o
COBJS = src/cmain.o
EDCXXFLAGS = -I ./ -I ./include/ -Wall -pthread $(CXXFLAGS)
EDCFLAGS = -I ./ -I ./include/ -Wall -pthread $(CFLAGS)
EDLDFLAGS := -lpthread -lm $(LDFLAGS)
CTARGET = c_test.out
CPPTARGET = cpp_test.out

all: cmain ccmain cppmain

cmain: 
	$(CC) $(EDCFLAGS) src/$@.c -o $@.out $(EDLDFLAGS)
	./$@.out

ccmain:
	$(CXX) $(EDCFLAGS) src/$@.cpp -o $@.out $(EDLDFLAGS)
	./$@.out

cppmain:
	$(CXX) $(EDCFLAGS) src/$@.cpp -o $@.out $(EDLDFLAGS)
	./$@.out

%.o: %.cpp
	$(CXX) $(EDCXXFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(EDCFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	$(RM) *.out
	$(RM) *.o
	$(RM) src/*.o

.PHONY: spotless
	$(RM) *.data
## This is a simple Makefile 

# Define what compiler to use and the flags.
CXX=g++
CCFLAGS= -g -std=c++11 -Wall -Werror
LDLIBS= -lm
#all: test_list
all: proj1

###################################
# BEGIN SOLUTION
proj1: project1_solution.o list.o
	$(CXX) -o proj1 project1_solution.o list.o

project1_solution.o: project1_solution.cpp
	$(CXX) -c project1_solution.cpp $(CCFLAGS) $(LDLIBS)

list.o: list.cpp list.h
	$(CXX) -c list.cpp $(CCFLAGS) $(LDLIBS)

clean:
	rm -f core *.o proj1



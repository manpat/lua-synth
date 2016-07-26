GCC=g++

SFLAGS=-std=c++14 -Wall -g
SFLAGS+=-I/home/patrick/Development/libraries/fmodstudio/api/lowlevel/inc
LFLAGS=-llua -ldl -Wl,--rpath=. libfmod.so.6 -lSDL2 -pthread -g
SRC=$(shell find . -name "*.cpp" | egrep -v "old")
OBJ=$(SRC:%.cpp=%.o) 

parallelbuild:
	@make build -j8 --silent

build: $(OBJ)
	@echo "-- Linking --"
	@$(GCC) $(OBJ) $(LFLAGS) -obuild

%.o: %.cpp %.h
	@echo "-- Generating $@ --"
	@$(GCC) $(SFLAGS) -c $< -o $@

%.o: %.cpp
	@echo "-- Generating $@ --"
	@$(GCC) $(SFLAGS) -c $< -o $@

run: parallelbuild
	@echo "-- Running --"
	@ulimit -s 1000000 ; ./build

clean:
	@echo "-- Cleaning --"
	@rm -f *.o
GCC=g++
AR=ar

SFLAGS=-std=c++14 -Wall -g
LFLAGS=-llua -ldl -lSDL2 -lsndfile -pthread -g
SRC=$(shell find . -name "*.cpp" | egrep -v "old|synth|lib")
OBJ=$(SRC:%.cpp=%.o) 

parallelbuild:
	@make build -j8 --silent

build: $(OBJ) libsynth.a
	@echo "-- Linking --"
	@$(GCC) $(OBJ) $(LFLAGS) -L. -lsynth -obuild

libsynth.a: synth.o lib.o
	@echo "-- Generating libsynth.a --"
	@$(AR) rcs libsynth.a synth.o lib.o

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
	@rm -f *.o libsynth.a
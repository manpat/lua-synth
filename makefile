# WINDOWSBUILD=1

ifdef WINDOWSBUILD
	PREFIX=/opt/mxe/usr/bin/i686-w64-mingw32.static-
	BUILDSUFFIX=.exe
	ROOTPREFIX=/opt/mxe/usr/i686-w64-mingw32.static
	TARGETLIBS=-lopengl32
else
	ROOTPREFIX=/usr/local
	TARGETLIBS=-lGL -ldl
endif

GCC = $(PREFIX)g++
AR = $(PREFIX)ar

SFLAGS = -std=c++14 -Wall -g
LFLAGS = -L$(ROOTPREFIX)/lib
LFLAGS+= -llua -ldl -lSDL2 -lsndfile -pthread -g
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
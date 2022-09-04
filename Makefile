# Makefile

CC = gcc

RESFILE = cmud

LIBS = -lstdc++

INCLUDES = -I ./src

SOURCES = src/*.cpp
 
linux:	
	$(CC) -m32 -std=c++98 -pedantic -o $(RESFILE) $(SOURCES) $(INCLUDES) -D NDEBUG $(LIBS) > err.log 2>&1

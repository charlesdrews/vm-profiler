CC=gcc
CFLAGS=-Wall -Wextra
SOURCES=profiler.c
EXE=profiler

all: $(SOURCES) $(EXE)
	$(CC) $(CFLAGS) -o $(EXE) $(SOURCES)

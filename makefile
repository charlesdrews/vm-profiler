CC=gcc
CFLAGS=-Wall -Wextra
SOURCES=profiler.c
EXE=profiler

all: $(SOURCES)
	$(CC) $(CFLAGS) -o $(EXE) $(SOURCES)

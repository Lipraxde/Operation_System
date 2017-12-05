OUTEXE = sleeping_barber

.PHONY : all build clean

cc = gcc

all: build

build: main.c
	cc $^ -g -lpthread -o $(OUTEXE)

clean:
	rm $(OUTEXE)

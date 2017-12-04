OUTEXE = sleeping_barber

.PHONY : all build clean

all: build

build: main.c
	cc $^ -lpthread -o $(OUTEXE)

clean:
	rm $(OUTEXE)

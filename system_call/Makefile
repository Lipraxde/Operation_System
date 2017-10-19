OUTEXE = sys_call
OUTLOG = strace.log


.PHONY : all build strace clean

all: build strace

build: sys_call.c
	cc $^ -o $(OUTEXE)

strace:
	strace ./$(OUTEXE) > strace.log 2>&1

clean:
	rm $(OUTEXE)
	rm $(OUTLOG)

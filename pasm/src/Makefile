# A makefile to build a native arm version to run on linux

SRC = pasm.c pasmpp.c pasmexp.c pasmop.c pasmdot.c pasmstruct.c pasmmacro.c
HDR = pasm.h pasmdbg.h pru_ins.h

all:	pasm

pasm: $(SRC) $(HDR)
	gcc -Wall -D_UNIX_ $(SRC) -o pasm

install:	pasm
	cp pasm /usr/bin

clean:
	rm -f pasm

# THE END

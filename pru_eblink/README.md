# pru_eblink

The name "pru_eblink" is terrible, but I will change that someday.

This directory holds the pru firmware.  pru_eblink.p is the assembly source.

Everything else is test fixtures.

pru_eblink.c is C code that calls and runs the PRU code, but it requies the
xxx library, which is elsewhere.  I fixed bugs in it and modified it to add
a handful of things (at least I think I added some things, but maybe I just
fixed bugs).

The javascript files require the "pru" module, which I put together and you
should also be able to find elsewhere.

Rebuilding the pru code requires "pasm", the PRU assembler, which I will also
try to include elsewhere.  If not, I am retaining the pru_eblink.bin file,
which is the code to be loaded into the PRU.

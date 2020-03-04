# Tom's makefile for the node pru bindings.

CPPOPTS = -Wl -I/usr/include/node
//CPPOPTS = -Wnarrowing -I/usr/include/node

all:
	node-gyp configure build

install:
	npm install -g --unsafe-perm

pru.o:	pru.cpp
	c++ $(CPPOPTS) -c pru.cpp

# THE END

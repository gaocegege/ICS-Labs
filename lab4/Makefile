CC=gcc
CFLAGS=-Wall -O2
LCFLAGS=-O2
YIS=./y86sim

all: y86sim

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .bin .sim
.bin.sim:
	$(YIS) $*.bin > $*.sim

# These are the explicit rules for making y86asm and y86emu
y86sim:
	$(CC) $(CFLAGS) y86sim.c -o y86sim

yat:
	$(CC) $(CFLAGS) yat.c -o yat

clean:
	rm -f y86sim *.sim *~  



CC=gcc
CFLAGS=-Wall -m32 -O2
LCFLAGS = -O2
YAS=./y86asm

all: y86asm

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .ys .bin .yo
.ys.bin:
	$(YAS) $*.ys
.ys.yo:
	$(YAS) -v $*.ys > $*.yo

# These are the explicit rules for making y86asm and y86emu
y86asm:
	$(CC) $(CFLAGS) y86asm.c -o y86asm

yat:
	$(CC) $(CFLAGS) yat.c -o yat

clean:
	rm -f *.o *.yo *.bin y86asm *~  



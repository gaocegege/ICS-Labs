CC=gcc
CFLAGS=-Wall -O2

YIS=../y86sim

INSFILES = halt.sim nop.sim rrmovl.sim cmovle.sim cmovl.sim cmove.sim cmovne.sim cmovge.sim cmovg.sim irmovl.sim rmmovl.sim mrmovl.sim addl.sim subl.sim andl.sim xorl.sim jmp.sim jle.sim jl.sim je.sim jne.sim jge.sim jg.sim call.sim ret.sim pushl.sim popl.sim byte.sim word.sim long.sim pos.sim align.sim

all: sim

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .bin .sim
.bin.sim:
	$(YIS) $*.bin > $*.sim

sim: $(INSFILES)

clean:
	rm -f *.sim *.base *~

CC=gcc
CFLAGS=-Wall -O2

YAS=./y86asm-base

APPFILES = abs-asum-cmov.yo abs-asum-jmp.yo asum.yo asumr.yo cjr.yo j-cc.yo poptest.yo pushquestion.yo pushtest.yo prog1.yo prog2.yo prog3.yo prog4.yo prog5.yo prog6.yo prog7.yo prog8.yo prog9.yo prog10.yo ret-hazard.yo

INSFILES = halt.yo nop.yo rrmovl.yo cmovle.yo cmovl.yo cmove.yo cmovne.yo cmovge.yo cmovg.yo irmovl.yo rmmovl.yo mrmovl.yo addl.yo subl.yo andl.yo xorl.yo jmp.yo jle.yo jl.yo je.yo jne.yo jge.yo jg.yo call.yo ret.yo pushl.yo popl.yo byte.yo word.yo long.yo pos.yo align.yo

all: yo 

# These are implicit rules for making .bin and .yo files from .ys files.
# E.g., make sum.bin or make sum.yo
.SUFFIXES: .ys .bin .yo
.ys.bin:
	$(YAS) $*.ys
.ys.yo:
	$(YAS) -v $*.ys > $*.yo

yo: $(INSFILES) $(APPFILES)

clean:
	rm -f *.yo *~ *.bin

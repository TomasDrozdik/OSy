# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Charles University

### Compiler, assembler and linker options
#
# All the options are described in detail in the GCC
# and binutils documentation. A brief description of
# some of the specific ones:
#
# -O2                         ... basic optimizations
# -march=r4000                ... generate code for MIPS R4000
# -mabi=32                    ... use standard 32 bit ABI
# -mgp32                      ... general purpose registers are 32 bit
# -msoft-float                ... do not generate floating point instructions
# -mlong32                    ... ints, long and pointers are 32 bit
# -G 0                        ... threshold for putting objects into small data/bss sections
# -mno-abicalls               ... do not generate SVR4-style position independent code
# -fno-pic                    ... do not generate position independent code using $gp
# -fno-builtin                ... do not recognize built-in functions without __builtin_ prefix
# -ffreestanding              ... a compilation without standard library and main()
# -nostdlib                   ... do not look for standard library in system directories
# -nostdinc                   ... do not look for standard header files in system directories
#


-include ../../config.mk

CCFLAGS = -I$(SRC_BASE)include -O2 -march=r4000 -mabi=32 -mgp32 -msoft-float -mlong32 -G 0 -mno-abicalls -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=c11
ASFLAGS = -I$(SRC_BASE)include -march=r4000 -mabi=32 -mgp32 -msoft-float -mlong32 -G 0 -mno-abicalls -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=gnu99 -I. -D__ASM__
LDFLAGS = -G 0 -static -g

### Libc sources and objects

LIBC_SOURCES = \
	src/entry.S \
	src/exit.c \
	src/heap.c \
	src/main.c \
	src/print.c \
	src/proc.c \
	src/runtime.c \
	src/stackspace.S

LIBC_OBJECTS := $(addsuffix .o,$(basename $(LIBC_SOURCES)))

DEPS := $(addsuffix .dep,$(basename $(LIBC_SOURCES)))

.PHONY: .FORCE all clean distclean dir-tree

all: libc.a

dir-tree:
	for i in $(LIBC_OBJECTS); do mkdir -p `dirname $$i`; done

distclean: clean
	rm -f libc.a

clean:
	find . -\( -name '*.o' -o -name '*.dep' -\) -exec rm -f \{\} \;

-include $(DEPS)

### Library assembly

libc.a: $(LIBC_OBJECTS)
	$(AR) rc $@ $(LIBC_OBJECTS)

### Default patterns

%.o: %.c | dir-tree
	$(CC) $(CCFLAGS) $(KERNEL_EXTRA_CFLAGS) -MD -MF $*.dep -c -o $@ $<

%.o: %.S | dir-tree
	$(CC) $(ASFLAGS) -c -o $@ $<
